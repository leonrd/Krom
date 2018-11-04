let project = new Project('Krom');

project.cpp11 = true;
project.addFile('Sources/**');

if (platform === Platform.WindowsApp) {
	project.addExclude('Sources/debug*');
	await project.addProject('Chakra/lib/Common/Codex');
	project.addIncludeDirs('Chakra/lib/Common/Codex');
}

if (platform !== Platform.WindowsApp) {
	await project.addProject('Chakra/Build');
}

project.setDebugDir('Deployment');

resolve(project);
