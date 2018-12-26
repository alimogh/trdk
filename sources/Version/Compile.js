/**************************************************************************
 *   Created: 2013/09/08 17:20:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/


//////////////////////////////////////////////////////////////////////////

var shell = new ActiveXObject("WScript.Shell");
var fileSystem = new ActiveXObject("Scripting.FileSystemObject");
var inputFile = "";
var outputDir = "";
var version = null;

eval('branch = ' +  fileSystem.OpenTextFile('Branch.json', 1).ReadAll());

//////////////////////////////////////////////////////////////////////////

function LoadArgs() {
	for (i = 0; i < WScript.Arguments.length; ++i) {
		var arg = WScript.Arguments(i);
		var opt = arg.substring(0, arg.indexOf("="));
		if (opt.length == 0) {
			throw "Failed to get required arguments";
		}
		if (opt == "InputFile") {
			inputFile = arg.substring(opt.length + 1, arg.length);
		} else if (opt == "OutputDir") {
			outputDir = arg.substring(opt.length + 1, arg.length);
		} else {
			throw "Unknown argument";
		}
	}
	if (inputFile == "" || outputDir == "") {
		throw "Failed to get required arguments";
	}
}

function IsExistsInFile(filePath, line) {
	try {
		var f = fileSystem.OpenTextFile(filePath, 1, false);
		while (f.AtEndOfStream != true) {
			if (f.ReadLine() == line) {
				return true;
			}
		}
	} catch (ex) {
	}
	return false;
}

function CompileVersion() {
	var source = fileSystem.OpenTextFile(inputFile, 1, false);
	if (source.AtEndOfStream == true) {
		throw "Could not find version file";
	}
	var expression = /^\s*(\d+)\.(\d+)\.(\d+)\s*$/;
	var match = expression.exec(source.ReadLine());
	if (!match) {
		throw "Could not parse version file";
	}
	var result = fileSystem.CreateTextFile(outputDir + "Version.json", true);
	result.WriteLine("{");
	result.WriteLine('  "release": ' + match[1] + ',');
	result.WriteLine('  "build": ' + match[2] + ',');
	result.WriteLine('  "status": ' + match[3]);
	result.WriteLine("}");
	result.Close();
	eval('version = ' + fileSystem.OpenTextFile(outputDir + 'Version.json', 1).ReadAll());
}

//////////////////////////////////////////////////////////////////////////

function CreateVersionCppHeaderFile() {

	var versionReleaseLine		= "#define TRDK_VERSION_RELEASE " + version.release;
	var versionBuildLine		= "#define TRDK_VERSION_BUILD " + version.build;
	var versionStatusLine		= "#define TRDK_VERSION_STATUS " + version.status;

	var versionBranchLine		= "#define TRDK_VERSION_BRANCH \"" + branch.name + "\"";
	var versionBranchLineW		= "#define TRDK_VERSION_BRANCH_W L\"" + branch.name + "\"";

	var vendorLine				= "#define TRDK_VENDOR \"" + branch.vendorName + "\"";
	var vendorLineW				= "#define TRDK_VENDOR_W L\"" + branch.vendorName + "\"";

	var domainLine				= "#define TRDK_DOMAIN \"" + branch.domain + "\"";
	var domainLineW				= "#define TRDK_DOMAIN_W L\"" + branch.domain + "\"";

	var supportEmailLine		= "#define TRDK_SUPPORT_EMAIL \"" + branch.supportEmail + "\"";
	var supportEmailLineW		= "#define TRDK_SUPPORT_EMAIL_W L\"" + branch.supportEmail + "\"";

	var licenseServiceSubdomainLine = "#define TRDK_LICENSE_SERVICE_SUBDOMAIN \"" + branch.licenseServiceSubdomain + "\"";
	var licenseServiceSubdomainLineW = "#define TRDK_LICENSE_SERVICE_SUBDOMAIN_W L\"" + branch.licenseServiceSubdomain + "\"";
	
	var nameLine				= "#define TRDK_NAME \"" + branch.productName + "\"";
	var nameLineW				= "#define TRDK_NAME_W L\"" + branch.productName + "\"";
	
	var copyrightLine			= "#define TRDK_COPYRIGHT \"" + branch.copyright + "\"";
	var copyrightLineW = "#define TRDK_COPYRIGHT_W L\"" + branch.copyright + "\"";

	var concurrencyProfileLineDebug = "#define TRDK_CONCURRENCY_PROFILE_DEBUG (::trdk::Lib::Concurrency::" + branch.concurrencyProfileDebug + ")"
	var concurrencyProfileLineTest = "#define TRDK_CONCURRENCY_PROFILE_TEST (::trdk::Lib::Concurrency::" + branch.concurrencyProfileTest + ")"
	var concurrencyProfileLineRelease = "#define TRDK_CONCURRENCY_PROFILE_RELEASE (::trdk::Lib::Concurrency::" + branch.concurrencyProfileRelease + ")"

	var requiredModulesLine = '';
	for (var i = 0; i < branch.requiredModules.length; ++i) {
		requiredModulesLine += '"' + branch.requiredModules[i] + '", ';
	}
	requiredModulesLine = '#define TRDK_GET_REQUIRED_MODUE_FILE_NAME_LIST() {' + requiredModulesLine + '};';

	var versionFilePath = outputDir + "Version.h";
	if (
			IsExistsInFile(versionFilePath, versionReleaseLine)
			&& IsExistsInFile(versionFilePath, versionBuildLine)
			&& IsExistsInFile(versionFilePath, versionStatusLine)
			&& IsExistsInFile(versionFilePath, versionBranchLine)
			&& IsExistsInFile(versionFilePath, vendorLine)
			&& IsExistsInFile(versionFilePath, domainLine)
			&& IsExistsInFile(versionFilePath, supportEmailLine)
			&& IsExistsInFile(versionFilePath, licenseServiceSubdomainLine)
			&& IsExistsInFile(versionFilePath, nameLine)
			&& IsExistsInFile(versionFilePath, copyrightLine)
			&& IsExistsInFile(versionFilePath, concurrencyProfileLineDebug)
			&& IsExistsInFile(versionFilePath, concurrencyProfileLineTest)
			&& IsExistsInFile(versionFilePath, concurrencyProfileLineRelease)
			&& IsExistsInFile(versionFilePath, requiredModulesLine)) {
		return;
	}

	var versionFile = fileSystem.CreateTextFile(versionFilePath, true);
	versionFile.WriteLine("");
	versionFile.WriteLine("#pragma once");
	versionFile.WriteLine("");
	versionFile.WriteLine(versionReleaseLine);
	versionFile.WriteLine(versionBuildLine);
	versionFile.WriteLine(versionStatusLine);
	versionFile.WriteLine("");
	versionFile.WriteLine(versionBranchLine);
	versionFile.WriteLine(versionBranchLineW);
	versionFile.WriteLine("");
	versionFile.WriteLine(vendorLine);
	versionFile.WriteLine(vendorLineW);
	versionFile.WriteLine("");
	versionFile.WriteLine(domainLine);
	versionFile.WriteLine(domainLineW);
	versionFile.WriteLine("");
	versionFile.WriteLine(supportEmailLine);
	versionFile.WriteLine(supportEmailLineW);
	versionFile.WriteLine("");
	versionFile.WriteLine(licenseServiceSubdomainLine);
	versionFile.WriteLine(licenseServiceSubdomainLineW);
	versionFile.WriteLine("");
	versionFile.WriteLine(nameLine);
	versionFile.WriteLine(nameLineW);
	versionFile.WriteLine("");
	versionFile.WriteLine(copyrightLine);
	versionFile.WriteLine(copyrightLineW);
	versionFile.WriteLine("");
	versionFile.WriteLine(concurrencyProfileLineDebug);
	versionFile.WriteLine(concurrencyProfileLineTest);
	versionFile.WriteLine(concurrencyProfileLineRelease);
	versionFile.WriteLine("");
	versionFile.WriteLine(requiredModulesLine);
	versionFile.WriteLine("");

}

//////////////////////////////////////////////////////////////////////////

try {
	LoadArgs();
	CompileVersion();
	CreateVersionCppHeaderFile();
} catch (ex) {
	shell.Popup("Version compiling: " + ex.message);
	throw "Failed to compile Version";
}

//////////////////////////////////////////////////////////////////////////
