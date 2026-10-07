function p = isarParamBlockPath(mdl, oriModelName, layerIdx, rowIdx)
% ISARPARAMBLOCKPATH  Constant block holding row rowIdx of the weight matrix of
% layer layerIdx, or the bias vector of that layer when rowIdx is 0.
%
% The weight paths follow weightBlockPath in fastRobOpen.m (AutoPatch).
if contains(oriModelName, 'WT') || contains(oriModelName, 'ACC')
    prefix = [mdl '/Feed-Forward Neural Network/Layer ' num2str(layerIdx)];
elseif contains(oriModelName, 'AFC')
    prefix = [mdl '/Air Fuel Control Model 1/Feed-Forward Neural Network/Layer ' num2str(layerIdx)];
elseif contains(oriModelName, 'SC')
    prefix = [mdl '/Subsystem/Feed-Forward Neural Network/Layer ' num2str(layerIdx)];
else
    error('isarParamBlockPath:unknownModel', 'No known block layout for %s', oriModelName);
end

if rowIdx == 0
    p = [prefix '/b{' num2str(layerIdx) '}'];
elseif layerIdx == 1
    p = [prefix '/IW{1,1}/IW{1,1}(' num2str(rowIdx) ',:)'''];
else
    p = [prefix '/LW{' num2str(layerIdx) ',' num2str(layerIdx-1) '}/IW{' ...
         num2str(layerIdx) ',' num2str(layerIdx-1) '}(' num2str(rowIdx) ',:)'''];
end
end
