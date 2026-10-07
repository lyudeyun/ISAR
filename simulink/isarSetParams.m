function isarSetParams(h, oriModelName, W, b)
% ISARSETPARAMS  Write all weights and biases of the NN controller into the
% loaded model of a fastRob session.
%
% W{l} and b{l} are the weight matrix and bias vector of layer l. Values are
% written with %.17g, which round-trips a double exactly, so writing the
% original parameters leaves the model numerically unchanged.
for l = 1:numel(W)
    for r = 1:size(W{l}, 1)
        set_param(isarParamBlockPath(h.mdl, oriModelName, l, r), 'Value', vecString(W{l}(r, :)));
    end
    set_param(isarParamBlockPath(h.mdl, oriModelName, l, 0), 'Value', vecString(b{l}));
end
end


function s = vecString(v)
s = ['[' strjoin(arrayfun(@(x) sprintf('%.17g', x), v(:)', 'UniformOutput', false), ';') ']'];
end
