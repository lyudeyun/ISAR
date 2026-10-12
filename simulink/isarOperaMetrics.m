function [names, m] = isarOperaMetrics(bench, X, sigNames)
% ISAROPERAMETRICS  Per-test-case quantities of the metrics of OPERA's rep_eval
% (AutoRepair_Alpha test/rep_eval_acc.py, rep_eval_afc.py, rep_eval_wt.py, commit
% a317187fb), computed as there from the trajectory of one test case.
%
%   names = isarOperaMetrics(bench)          names of the quantities of bench
%   [names, m] = isarOperaMetrics(bench, X, sigNames)
%       m(k) is quantity names{k} of the trajectory X (one column per signal,
%       in the order of sigNames). An empty X (failed simulation) gives NaN.
%
% The metrics follow from these over all test cases, see evaluate_testset.py.
% The signals it needs are recorded by isar_sl when cfg.metrics is set.

switch bench
    case 'ACC'
        names = {'x_ego', 'steady', 'delta_v_sum', 'delta_v_time', 'comfortable', 'unsteady', 'recovery'};
    case 'AFC'
        names = {'steady', 'unsteady', 'recovery', 'MAE', 'MAXERR'};
    case 'WT'
        names = {'steady', 'MAE', 'MAXERR'};
    otherwise
        error('isarOperaMetrics:bench', 'no OPERA metrics for %s', bench);
end
if nargin < 2
    return;
end
m = nan(1, numel(names));
if isempty(X)
    return;
end
sig = @(name) X(:, strcmp(sigNames, name))';

switch bench
    case 'ACC'
        steady_const = 2;
        delta_a_ego_const = 0.1;
        resilience_const = 10;
        v_set = 30;
        x_ego_sig = sig('x_ego');
        steady_state = sig('d_rel') - 1.4 * sig('v_ego') - 10 - steady_const;
        steady_idx = find(steady_state > 0);
        safety_state = sig('d_rel') - 1.4 * sig('v_ego') - 10;
        safety_idx = find(safety_state > 0);
        v_ego_sig = sig('v_ego');
        a_ego_sig = sig('a_ego');
        comfortable_idx = find(abs(diff(a_ego_sig)) < delta_a_ego_const);
        [unsteady_num, recovery_num] = unsteadyIntervals(find(steady_state < 0), resilience_const);
        m = [x_ego_sig(1, end), numel(steady_idx) / 501, sum(abs(v_ego_sig(safety_idx) - v_set)), ...
             numel(safety_idx), numel(comfortable_idx) / 500, unsteady_num, recovery_num];
    case 'AFC'
        steady_const = 0.10;
        resilience_const = 10;
        mu_sig = abs(ones(size(sig('AF'))) - sig('AF') / 14.7);
        steady_idx = find(mu_sig <= steady_const);
        [unsteady_count, recovery_count] = unsteadyIntervals(find(mu_sig > steady_const), resilience_const);
        m = [numel(steady_idx) / numel(mu_sig), unsteady_count, recovery_count, mean(mu_sig), max(mu_sig)];
    case 'WT'
        steady_const = 0.5;
        error_sig = sig('h_error');
        sel_error_sig = abs([error_sig(1, 41:50), error_sig(1, 91:100), error_sig(1, 141:150)]);
        m = [numel(find(sel_error_sig < steady_const)) / numel(sel_error_sig), mean(sel_error_sig), max(sel_error_sig)];
end
end


function [unsteady_num, recovery_num] = unsteadyIntervals(unsteady_idx, resilience_const)
% Number of runs of consecutive indices in unsteady_idx, and of those shorter
% than resilience_const, as in rep_eval
if numel(unsteady_idx) == 0 || numel(unsteady_idx) == 1
    unsteady_num = numel(unsteady_idx);
    recovery_num = numel(unsteady_idx);
    return;
end
inter = find(diff(unsteady_idx) ~= 1);
edges = [0, inter, numel(unsteady_idx)];
lengths = diff(edges);
unsteady_num = numel(lengths);
recovery_num = sum(lengths < resilience_const);
end
