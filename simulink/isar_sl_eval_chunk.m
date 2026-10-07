function rob = isar_sl_eval_chunk(mdlInfo, simConf, constParams, normParams, W, b, inputs)
% ISAR_SL_EVAL_CHUNK  Robustness of a list of test cases for one candidate
% controller, on the current MATLAB process (a pool worker or the client).
%
% The fastRob session is built on the first call and reused afterwards.
assignStructToBase(constParams);
assignStructToBase(normParams);
h = fastRobOpen(mdlInfo, simConf);
isarSetParams(h, mdlInfo.faultyMdl, W, b);
rob = zeros(1, numel(inputs));
for t = 1:numel(inputs)
    rob(t) = fastRob(h, inputs{t});
end
end
