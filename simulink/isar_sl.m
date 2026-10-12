function out = isar_sl(action, varargin)
% ISAR_SL  MATLAB side of the ISAR interface to Simulink benchmarks.
%
%   info = isar_sl('init', cfg)
%       Sets up paths, workspace constants, the parallel pool and the test
%       suite, and returns the controller's original parameters. cfg fields:
%         benchDir, model, nnFile, nnStru, phiStr, params (cellstr of
%         'name=value;'), inName (cellstr), inSpan, testSuiteFile, normFile,
%         breachPath, workDir, workers
%       metrics (optional): 'ACC', 'AFC' or 'WT' to record the signals of OPERA's
%         metrics, for 'eval_metrics'
%   rob = isar_sl('eval', Wflat, bflat, caseIdx)
%       Robustness of the test cases caseIdx (1-based) for the controller
%       given by the flattened weights and biases (column-major per layer,
%       layers concatenated, same layout as returned by 'init').
%   r = isar_sl('eval_metrics', Wflat, bflat, caseIdx)
%       As 'eval', plus the per-test-case quantities of OPERA's metrics:
%       r.rob, r.names and r.M (one row per test case), see isarOperaMetrics.
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
                     'workers', S.workers, 'workerPids', S.workerPids);

    case {'eval', 'eval_metrics'}
        assert(~isempty(S), 'isar_sl:notInitialized', 'call isar_sl(''init'', cfg) first');
        [Wflat, bflat, caseIdx] = varargin{:};
        W = unflattenCells(Wflat, S.Wshape);
        b = unflattenCells(bflat, S.bshape);
        inputs = S.inputs(caseIdx(:)');
        if strcmp(action, 'eval')
            out = evalCases(S, W, b, inputs, '');
        else
            assert(~isempty(S.metrics), 'isar_sl:noMetrics', 'init was called without cfg.metrics');
            R = evalCases(S, W, b, inputs, S.metrics);
            out = struct('rob', R(:, 1)', 'names', {isarOperaMetrics(S.metrics)}, 'M', R(:, 2:end));
        end

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
% OPERA's metrics need signals besides those of the specification, recorded as in its rep_eval
% (simConf.extraSigs). 'eval_metrics' simulates with them in a session of its own; 'eval' keeps S.simConf
% and so the robustness as without metrics.
S.metrics = '';
S.metricsSimConf = S.simConf;
if isfield(cfg, 'metrics') && ~isempty(cfg.metrics)
    S.metrics = char(cfg.metrics);
    switch S.metrics
        case 'ACC', S.metricsSimConf.extraSigs = {'x_ego', 'd_rel', 'v_ego', 'a_ego'};
        case 'AFC', S.metricsSimConf.extraSigs = {'AF'};
        case 'WT',  S.metricsSimConf.extraSigs = {'h_error'};
        otherwise, error('isar_sl:metrics', 'no OPERA metrics for %s', S.metrics);
    end
end

ts = load(fullfile(cfg.benchDir, cfg.testSuiteFile), 'testSuite');
S.inputs = {ts.testSuite.testCases.sysInputs};
S.storedRob = [ts.testSuite.testCases.rob];

% fastRobOpen writes model copies into the current folder of each process
if ~isfolder(cfg.workDir), mkdir(cfg.workDir); end
cd(cfg.workDir);

S.workers = double(cfg.workers);
S.workerPids = zeros(1, 0);
if S.workers > 1
    initParpool(S.workers);
    % Each worker works in its own folder, see isarWorkerDir.
    % Python kills these processes, along with this one, if an evaluation hangs
    S.workerPids = double(reshape(fetchOutputs(parfevalOnAll(@isarWorkerDir, 1, cfg.workDir)), 1, []));
end
end


function rob = evalCases(S, W, b, inputs, metrics)
% Robustness of inputs as a row; with metrics, one row per input: robustness, then the quantities of isarOperaMetrics
if S.workers > 1 && numel(inputs) > 1
    nChunk = min(S.workers, numel(inputs));
    edges = round(linspace(0, numel(inputs), nChunk + 1));
    chunks = cell(1, nChunk);
    for ci = 1:nChunk
        chunks{ci} = inputs(edges(ci)+1:edges(ci+1));
    end
    robChunks = cell(1, nChunk);
    mdlInfo = S.mdlInfo; constParams = S.constParams; normParams = S.normParams;
    simConf = S.simConf;
    if ~isempty(metrics), simConf = S.metricsSimConf; end
    parfor ci = 1:nChunk
        robChunks{ci} = isar_sl_eval_chunk(mdlInfo, simConf, constParams, normParams, W, b, chunks{ci}, metrics);
    end
    if isempty(metrics)
        rob = [robChunks{:}];
    else
        rob = vertcat(robChunks{:});
    end
else
    simConf = S.simConf;
    if ~isempty(metrics), simConf = S.metricsSimConf; end
    rob = isar_sl_eval_chunk(S.mdlInfo, simConf, S.constParams, S.normParams, W, b, inputs, metrics);
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
