function rob = isar_sl_eval_chunk(mdlInfo, simConf, constParams, normParams, W, b, inputs, metrics)
% ISAR_SL_EVAL_CHUNK  Robustness of a list of test cases for one candidate
% controller, on the current MATLAB process (a pool worker or the client).
%
% The fastRob session is built on the first call and reused afterwards.
% With metrics ('ACC', 'AFC' or 'WT'), rob has one row per test case: the
% robustness, then the quantities of OPERA's metrics (isarOperaMetrics).
assignStructToBase(constParams);
assignStructToBase(normParams);
h = fastRobOpen(mdlInfo, simConf);
isarSetParams(h, mdlInfo.faultyMdl, W, b);
if nargin < 8 || isempty(metrics)
    rob = zeros(1, numel(inputs));
    for t = 1:numel(inputs)
        rob(t) = fastRob(h, inputs{t});
    end
    return;
end
rob = nan(numel(inputs), 1 + numel(isarOperaMetrics(metrics)));
for t = 1:numel(inputs)
    [r, tout, X] = fastRob(h, inputs{t});
    % the metrics index time steps, so they need the trajectory on the grid of the specification
    if ~isempty(tout) && numel(tout) ~= numel(h.tspan)
        error('isar_sl:timeGrid', 'test case %d: %d samples, expected %d', t, numel(tout), numel(h.tspan));
    end
    [~, m] = isarOperaMetrics(metrics, X, h.sigNames);
    rob(t, :) = [r, m];
end
end
