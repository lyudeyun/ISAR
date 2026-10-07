function out = isar_sl(action, varargin)
% ISAR_SL  MATLAB side of the ISAR interface to Simulink benchmarks.
%
%   info = isar_sl('init', cfg)
%       Sets up paths, workspace constants, the parallel pool and the test
%       suite, and returns the controller's original parameters. cfg fields:
%         benchDir, model, nnFile, nnStru, phiStr, params (cellstr of
%         'name=value;'), inName (cellstr), inSpan, testSuiteFile, normFile,
%         breachPath, workDir, workers
%   rob = isar_sl('eval', Wflat, bflat, caseIdx)
%       Robustness of the test cases caseIdx (1-based) for the controller
%       given by the flattened weights and biases (column-major per layer,
%       layers concatenated, same layout as returned by 'init').
%   p = isar_sl('read_model_params')
%       Weights and biases currently stored in the model blocks, flattened
%       like Wflat/bflat. Used to check that the model matches the network file.
%   isar_sl('close')
%       Takes the models out of fast restart on the client and the workers.

persistent S

switch action
    case 'init'
        S = initSession(varargin{1});
        out = struct('Wflat', flattenCells(S.W), 'bflat', flattenCells(S.b), ...
                     'Wshape', S.Wshape, 'bshape', S.bshape, ...
                     'numCases', numel(S.inputs), 'storedRob', S.storedRob, ...
                     'workers', S.workers);

    case 'eval'
        assert(~isempty(S), 'isar_sl:notInitialized', 'call isar_sl(''init'', cfg) first');
        [Wflat, bflat, caseIdx] = varargin{:};
        W = unflattenCells(Wflat, S.Wshape);
        b = unflattenCells(bflat, S.bshape);
        inputs = S.inputs(caseIdx(:)');
        out = evalCases(S, W, b, inputs);

    case 'read_model_params'
        assert(~isempty(S), 'isar_sl:notInitialized', 'call isar_sl(''init'', cfg) first');
        out = readModelParams(S);

    case 'close'
        fastRobClose();
        out = [];

    otherwise
        error('isar_sl:unknownAction', 'unknown action %s', action);
end
end


function S = initSession(cfg)
here = fileparts(mfilename('fullpath'));
addpath(here, fullfile(here, 'fastrob'), cfg.benchDir);
addpath(genpath(cfg.breachPath));
InitBreach;

S.constParams = parseParams(cfg.params);
norm = load(cfg.normFile);
S.normParams = struct('is_nor', norm.is_nor, 'x_gain', norm.x_gain, 'x_offset', norm.x_offset, ...
                      'y_gain', norm.y_gain, 'y_offset', norm.y_offset);

nnData = load(fullfile(cfg.benchDir, cfg.nnFile), 'net');
net = nnData.net;
nnStru = double(cfg.nnStru(:)');
assert(net.numLayers == numel(nnStru), 'isar_sl:structure', ...
    'network has %d layers, nnStru has %d', net.numLayers, numel(nnStru));
S.W = cell(1, net.numLayers);
S.b = cell(1, net.numLayers);
for l = 1:net.numLayers
    if l == 1
        S.W{l} = net.IW{1,1};
    else
        S.W{l} = net.LW{l,l-1};
    end
    S.b{l} = net.b{l};
    assert(size(S.W{l}, 1) == nnStru(l), 'isar_sl:structure', ...
        'layer %d has %d neurons, nnStru says %d', l, size(S.W{l}, 1), nnStru(l));
end
S.Wshape = cell2mat(cellfun(@size, S.W(:), 'UniformOutput', false));
S.bshape = cell2mat(cellfun(@size, S.b(:), 'UniformOutput', false));

% fastRobOpen validates every listed weight against the model, so list all of them
sps = zeros(0, 5);
for l = 1:numel(S.W)
    [rr, cc] = ndgrid(1:size(S.W{l}, 1), 1:size(S.W{l}, 2));
    sps = [sps; repmat(l, numel(rr), 1), rr(:), cc(:), S.W{l}(:), zeros(numel(rr), 1)]; %#ok<AGROW>
end
S.mdlInfo = struct('faultyMdl', cfg.model, 'spsWeights', sps, 'oriWeights', {S.W});

S.simConf = struct('T', S.constParams.T, 'Ts', S.constParams.Ts, ...
                   'in_name', {cellstr(cfg.inName)}, 'in_span', double(cfg.inSpan(:)'), ...
                   'phi', STL_Formula('phi', cfg.phiStr));

ts = load(fullfile(cfg.benchDir, cfg.testSuiteFile), 'testSuite');
S.inputs = {ts.testSuite.testCases.sysInputs};
S.storedRob = [ts.testSuite.testCases.rob];

% fastRobOpen writes model copies into the current folder of each process
if ~isfolder(cfg.workDir), mkdir(cfg.workDir); end
cd(cfg.workDir);

S.workers = double(cfg.workers);
if S.workers > 1
    initParpool(S.workers);
    wait(parfevalOnAll(@cd, 0, cfg.workDir));
end
end


function rob = evalCases(S, W, b, inputs)
if S.workers > 1 && numel(inputs) > 1
    nChunk = min(S.workers, numel(inputs));
    edges = round(linspace(0, numel(inputs), nChunk + 1));
    chunks = cell(1, nChunk);
    for ci = 1:nChunk
        chunks{ci} = inputs(edges(ci)+1:edges(ci+1));
    end
    robChunks = cell(1, nChunk);
    mdlInfo = S.mdlInfo; simConf = S.simConf; constParams = S.constParams; normParams = S.normParams;
    parfor ci = 1:nChunk
        robChunks{ci} = isar_sl_eval_chunk(mdlInfo, simConf, constParams, normParams, W, b, chunks{ci});
    end
    rob = [robChunks{:}];
else
    rob = isar_sl_eval_chunk(S.mdlInfo, S.simConf, S.constParams, S.normParams, W, b, inputs);
end
end


function p = readModelParams(S)
assignStructToBase(S.constParams);
assignStructToBase(S.normParams);
h = fastRobOpen(S.mdlInfo, S.simConf);
W = S.W; b = S.b;
for l = 1:numel(W)
    for r = 1:size(W{l}, 1)
        W{l}(r, :) = str2num(get_param(isarParamBlockPath(h.mdl, S.mdlInfo.faultyMdl, l, r), 'Value')); %#ok<ST2NM>
    end
    b{l} = reshape(str2num(get_param(isarParamBlockPath(h.mdl, S.mdlInfo.faultyMdl, l, 0), 'Value')), size(b{l})); %#ok<ST2NM>
end
p = struct('Wflat', flattenCells(W), 'bflat', flattenCells(b));
end


function params = parseParams(lines)
% Lines such as 'T=50;' from the config's parameters entry
params = struct();
lines = cellstr(lines);
for i = 1:numel(lines)
    tok = regexp(strtrim(lines{i}), '^(\w+)\s*=\s*(.*?);?$', 'tokens', 'once');
    if isempty(tok), continue; end
    params.(tok{1}) = eval(tok{2});
end
end


function v = flattenCells(c)
v = cell2mat(cellfun(@(x) x(:)', c(:)', 'UniformOutput', false));
end


function c = unflattenCells(v, shapes)
v = double(v(:)');
c = cell(1, size(shapes, 1));
k = 0;
for l = 1:size(shapes, 1)
    n = prod(shapes(l, :));
    c{l} = reshape(v(k+1:k+n), shapes(l, :));
    k = k + n;
end
assert(k == numel(v), 'isar_sl:paramCount', 'got %d values, the network has %d', numel(v), k);
end
