function pid = isarWorkerDir(workDir)
% ISARWORKERDIR  Move the calling process into its own folder under workDir and
% return its process id.
%
% Models simulated in accelerator mode (OPERA's AFC) build code in slprj/ of the
% current folder; processes building into one slprj/ at the same time corrupt it
% ("created for a different release"), so every pool worker gets its own folder.
pid = feature('getpid');
d = fullfile(workDir, sprintf('worker_%d', pid));
if ~isfolder(d), mkdir(d); end
cd(d);
end
