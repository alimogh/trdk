//
//    Created: 2018/05/16 15:40
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

var qtMajorVersion = '5';
var qtSourceDir = 'C:/Qt/5.9.1/msvc2015_64/';
var qtSuffix = '';
var openSslSuffix = '';
var pocoSuffix = '';
var qxOrmSuffix = '';

var shell = new ActiveXObject("WScript.Shell");
var fileSystem = new ActiveXObject("Scripting.FileSystemObject");
var stdout = fileSystem.GetStandardStream(1);
var stderr = fileSystem.GetStandardStream(2);
var sourceDir = "";
var outputDir = "";
var intDir = "";
var solutionDir = "";
var projectDir = "";
var configuration = "";
var suffix = "";
var suffixHumanReadable = "";
var resultFile = "";
var version = null;

eval('branch = ' +  fileSystem.OpenTextFile('../Version/Branch.json', 1).ReadAll());

//////////////////////////////////////////////////////////////////////////

function LoadArgs() {
	for (i = 0; i < WScript.Arguments.length; ++i) {
		var arg = WScript.Arguments(i);
		var opt = arg.substring(0, arg.indexOf("="));
		if (opt.length == 0) {
			throw "Failed to get required arguments";
		}
		if (opt == "OutputDir") {
			outputDir = arg.substring(opt.length + 1, arg.length);
		} else if (opt == "IntDir") {
			intDir = arg.substring(opt.length + 1, arg.length) + '/Source';
		} else if (opt == "Configuration") {
			configuration = arg.substring(opt.length + 1, arg.length)
				.toLowerCase();
			if (configuration.substring(0, 7) == "release") {
				suffix = "";
				suffixHumanReadable = "";
			} else if (configuration.substring(0, 4) == "test") {
				suffix = "_test";
				suffixHumanReadable = ' (TEST)';
			} else {
				suffix = "_dbg";
				suffixHumanReadable = ' (DEBUG)';
				qtSuffix = 'd';
				openSslSuffix = 'd';
				qxOrmSuffix = 'd';
				pocoSuffix = 'd';
			}
		} else if (opt == "SourceDir") {
			sourceDir = arg.substring(opt.length + 1, arg.length);
		} else if (opt == "SolutionDir") {
			solutionDir = arg.substring(opt.length + 1, arg.length);
		} else if (opt == "ProjectDir") {
			projectDir = arg.substring(opt.length + 1, arg.length);
		} else {
			throw "Unknown argument";
		}
	}
	if (outputDir == "" || intDir == "" || sourceDir == "" || solutionDir == ""
			|| projectDir == "" || configuration == "") {
		throw "Failed to get required arguments";
	}
}

function LoadVersion() {
	eval('version = ' + fileSystem.OpenTextFile('../Version/Version.json', 1)
		.ReadAll());
}

function GenerateResultFileName() {
	resultFile = outputDir;
	resultFile += branch.productNameFile + '-setup-';
	resultFile += version.release + '.' + version.build + '.' + version.status;
	resultFile += suffix;
	resultFile += '.exe';
}

function Cleanup() {
	if (fileSystem.FileExists(intDir) || fileSystem.FolderExists(intDir)) {
		fileSystem.DeleteFolder(intDir, true);
	}
	if (fileSystem.FileExists(resultFile)
			|| fileSystem.FolderExists(resultFile)) {
		fileSystem.DeleteFile(resultFile, true);
	}
}

function CompileConfig(file) {
	file = intDir + '/' + file;
	var result = fileSystem.OpenTextFile(file, 1).ReadAll();
	result = result.replace(/%{Name}/g, branch.productName);
	result = result.replace(/%{NameFile}/g, branch.productNameFile);
	result = result.replace(/%{Suffix}/g, suffix);
	result = result.replace(/%{SuffixHumanReadable}/g, suffixHumanReadable);
	result = result.replace(/%{Publisher}/g, branch.vendorName);
	result = result.replace(/%{Version}/g, version.release + '.'
		+ version.build + '.' + version.status);
	{
		var now = new Date();
		var mm = now.getMonth() + 1;
		if (mm < 10) {
			mm = '0' + mm
		}
		result = result.replace(/%{ReleaseDate}/g, now.getFullYear()
			+ '-' + mm + '-' + now.getDate());
	}
	result = result.replace(/%{Domain}/g, branch.domain);
	// result = result.replace(/%{SolutionDir}/g, solutionDir);
	fileSystem.OpenTextFile(file, 2).Write(result);
}

function CreateStructure() {
	fileSystem.CreateFolder(intDir);
	if (!fileSystem.FolderExists(outputDir)) {
		fileSystem.CreateFolder(outputDir);
	}

	fileSystem.CopyFolder(projectDir + '/Config', intDir + '/Config', true);
	fileSystem.CopyFolder(projectDir + '/Packages', intDir + '/Packages', true);

	CompileConfig('Config/config.xml');
	CompileConfig('Config/Controller.qs');
	CompileConfig('Packages/com.robotdk.trdk/meta/package.xml');
	CompileConfig('Packages/com.robotdk.trdk/meta/Component.qs');
}

function CopyContent() {
	var destination = intDir + '/packages/com.robotdk.trdk/data/';
	if (!fileSystem.FolderExists(destination)) {
		fileSystem.CreateFolder(destination);
	}

	var isQtCoreRequired = true;
	var isQtSqlRequired = false;
	var isQtChartsRequired = false;
	var isPocoRequired = false;
	var isOpenSslRequired = false;
	var isQxOrmRequired = false;

	for (var i = 0; i < branch.requiredModules.length; ++i) {
		var module = branch.requiredModules[i];
		var file = sourceDir + '/' + module + suffix + '.dll';
		stdout.WriteLine('Adding module "' + module + '"...');
		fileSystem.CopyFile(file, destination, true);
		if (module == 'FrontEnd') {
			isQtCoreRequired = true;
			isQtSqlRequired = true;
			isQxOrmRequired = true;
		} else if (module == 'Shell') {
			isQtCoreRequired = true;
		} else if (module == 'Charts') {
			isQtCoreRequired = true;
			isQtChartsRequired = true;
		} else if (module == 'Rest') {
			isPocoRequired = true;
			isOpenSslRequired = true;
		}
	}

	fileSystem.CopyFile(
		sourceDir + '/' + branch.productNameFile + suffix + '.exe', destination,
		true);

	var qtLibsSourceDir = qtSourceDir + 'bin/';
	var qtPluginsSourceDir = qtSourceDir + 'plugins/';
	if (isQtCoreRequired) {
		stdout.WriteLine('Adding Qt Core DLLs...');
		fileSystem.CopyFile(qtLibsSourceDir + 'Qt' + qtMajorVersion + 'Core' + qtSuffix + '.dll', destination, true);
		fileSystem.CopyFile(qtLibsSourceDir + 'Qt' + qtMajorVersion + 'Gui' + qtSuffix + '.dll', destination, true);
		fileSystem.CopyFile(qtLibsSourceDir + 'Qt' + qtMajorVersion + 'Gui' + qtSuffix + '.dll', destination, true);
		fileSystem.CopyFile(qtLibsSourceDir + 'Qt' + qtMajorVersion + 'Widgets' + qtSuffix + '.dll', destination, true);
		fileSystem.CreateFolder(destination + 'platforms');
		fileSystem.CopyFile(qtPluginsSourceDir + 'platforms/qwindows' + qtSuffix + '.dll', destination + 'platforms/', true);
	}
	if (isQtSqlRequired) {
		stdout.WriteLine('Adding Qt SQL DLLs...');
		fileSystem.CopyFile(qtLibsSourceDir + 'Qt' + qtMajorVersion + 'Sql' + qtSuffix + '.dll', destination, true);
	}
	if (isQtChartsRequired) {
		stdout.WriteLine('Adding Qt Charts DLLs...');
		fileSystem.CopyFile(qtLibsSourceDir + 'Qt' + qtMajorVersion + 'Charts' + qtSuffix + '.dll', destination, true);
	}
	if (isPocoRequired) {
		stdout.WriteLine('Adding POCO DLLs...');
		var pocoDir = solutionDir + '/../externals/poco-1.7.9-all/bin64/';
		fileSystem.CopyFile(pocoDir + 'PocoCrypto64' + pocoSuffix + '.dll', destination, true);
		fileSystem.CopyFile(pocoDir + 'PocoFoundation64' + pocoSuffix + '.dll', destination, true);
		fileSystem.CopyFile(pocoDir + 'PocoJSON64' + pocoSuffix + '.dll', destination, true);
		fileSystem.CopyFile(pocoDir + 'PocoNet64' + pocoSuffix + '.dll', destination, true);
		fileSystem.CopyFile(pocoDir + 'PocoNetSSL64' + pocoSuffix + '.dll', destination, true);
		fileSystem.CopyFile(pocoDir + 'PocoUtil64' + pocoSuffix + '.dll', destination, true);
		fileSystem.CopyFile(pocoDir + 'PocoXML64' + pocoSuffix + '.dll', destination, true);
	}
	if (isOpenSslRequired) {
		stdout.WriteLine('Adding OpenSSL DLLs...');
		var openSslDir = solutionDir
			+ '/../externals/openssl-1.0.1t-vs2015/bin64/';
		fileSystem.CopyFile(openSslDir + 'libeay32MD' + openSslSuffix + '.dll', destination, true);
		fileSystem.CopyFile(openSslDir + 'ssleay32MD' + openSslSuffix + '.dll', destination, true);
	}
	if (isQxOrmRequired) {
		stdout.WriteLine('Adding QxORM DLLs...');
		fileSystem.CopyFile(solutionDir + '/../externals/QxOrm/lib/QxOrm'
			+ qxOrmSuffix + '.dll', destination, true);
	}

	stdout.WriteLine('Adding VC Runtime DLLs...');
	fileSystem.CopyFile('C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/redist/x64/Microsoft.VC140.CRT/vcruntime140.dll', destination, true);
	fileSystem.CopyFile('C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/redist/x64/Microsoft.VC140.CRT/msvcp140.dll', destination, true);

}

function Build() {
	var command
		= 'C:/Qt/Tools/QtInstallerFramework/3.0/bin/binarycreator.exe -c "'
		+ intDir + '/Config/config.xml" -p "' + intDir + '/Packages" "'
		+ resultFile;
	stdout.WriteLine(command);
	var result = shell.Run(command, 1, true);
	return result == 0;
}

var result = false;
try {
	LoadArgs();
	LoadVersion();
	GenerateResultFileName();
	Cleanup();
	CreateStructure();
	CopyContent();
	result = Build();
} catch (ex) {
	var message = ex.message;
	if (message == undefined) {
		message = ex;
	}
	stderr.WriteLine("Failed to compile Installer: " + message);
	WScript.Quit(1);
}
if (!result) {
	WScript.Quit(2);
}

stdout.WriteLine('Result: ' + resultFile);

WScript.Quit(0);