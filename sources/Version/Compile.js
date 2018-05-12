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
// Customization:

var branch = "Brutus"
var productName = "Brutus"
var vendorName = "Eugene V. Palchukovsky"
var domain = "robotdk.com"
var licenseServiceSubdomain = "licensing"
var supportEmail = "support@" + domain
var copyright = "Copyright 2018 (C) " + vendorName + ", " + domain + ". All rights reserved."
var concurrencyProfileDebug = "PROFILE_RELAX"
var concurrencyProfileTest = "PROFILE_RELAX"
var concurrencyProfileRelease = "PROFILE_RELAX"
var guaranteedUsageDaysNumber = 30
var numberOfDaysBeforeUsageStop = guaranteedUsageDaysNumber + 45;
var requiredModules = [
	'Core'
	, 'Engine'
	, 'FrontEnd'
	, 'Charts'
	, 'Rest'
	, 'Exmo'
	, 'ArbitrationAdvisor'
	, 'MarketMaker'
	, 'PingPong'
]

//////////////////////////////////////////////////////////////////////////

var shell = new ActiveXObject("WScript.Shell");
var fileSystem = new ActiveXObject("Scripting.FileSystemObject");
var inputFile = "";
var outputDir = "";

//////////////////////////////////////////////////////////////////////////

function GetArgs() {
	for (i = 0; i < WScript.Arguments.length; ++i) {
		var arg = WScript.Arguments(i);
		var opt = arg.substring(0, arg.indexOf("="));
		if (opt.length == 0) {
			return false;
		}
		if (opt == "InputFile") {
			inputFile = arg.substring(opt.length + 1, arg.length);
		} else if (opt == "OutputDir") {
			outputDir = arg.substring(opt.length + 1, arg.length);
		} else {
			return false;
		}
	}
	return inputFile != "" && outputDir != "";
}

function IsExistsInFile(filePath, line) {
	try {
		var f = fileSystem.OpenTextFile(filePath, 1, false);
		while (f.AtEndOfStream != true) {
			if (f.ReadLine() == line) {
				return true;
			}
		}
	} catch (e) {
		//...//
	}
	return false;
}

function GetVersion() {
	var f = fileSystem.OpenTextFile(inputFile, 1, false);
	if (f.AtEndOfStream != true) {
		var expression = /^\s*(\d+)\.(\d+)\.(\d+)\s*$/;
		var match = expression.exec(f.ReadLine());
		if (match) {
			var version = new Object;
			version.release = match[1];
			version.build = match[2];
			version.status = match[3];
			return version;
		} else {
			shell.Popup("Version compiling: could not parse version file.");
		}
	} else {
		shell.Popup("Version compiling: could not find version file.");
	}
	return null;
}

//////////////////////////////////////////////////////////////////////////

function CreateVersionCppHeaderFile() {

	var version = GetVersion();
	if (version == null) {
		return;
	}

	var versionReleaseLine		= "#define TRDK_VERSION_RELEASE " + version.release;
	var versionBuildLine		= "#define TRDK_VERSION_BUILD " + version.build;
	var versionStatusLine		= "#define TRDK_VERSION_STATUS " + version.status;

	var versionBranchLine		= "#define TRDK_VERSION_BRANCH \"" + branch + "\"";
	var versionBranchLineW		= "#define TRDK_VERSION_BRANCH_W L\"" + branch + "\"";

	var vendorLine				= "#define TRDK_VENDOR \"" + vendorName + "\"";
	var vendorLineW				= "#define TRDK_VENDOR_W L\"" + vendorName + "\"";

	var domainLine				= "#define TRDK_DOMAIN \"" + domain + "\"";
	var domainLineW				= "#define TRDK_DOMAIN_W L\"" + domain + "\"";

	var supportEmailLine		= "#define TRDK_SUPPORT_EMAIL \"" + supportEmail + "\"";
	var supportEmailLineW		= "#define TRDK_SUPPORT_EMAIL_W L\"" + supportEmail + "\"";

	var licenseServiceSubdomainLine = "#define TRDK_LICENSE_SERVICE_SUBDOMAIN \"" + licenseServiceSubdomain + "\"";
	var licenseServiceSubdomainLineW = "#define TRDK_LICENSE_SERVICE_SUBDOMAIN_W L\"" + licenseServiceSubdomain + "\"";
	
	var nameLine				= "#define TRDK_NAME \"" + productName + "\"";
	var nameLineW				= "#define TRDK_NAME_W L\"" + productName + "\"";
	
	var copyrightLine			= "#define TRDK_COPYRIGHT \"" + copyright + "\"";
	var copyrightLineW = "#define TRDK_COPYRIGHT_W L\"" + copyright + "\"";

	var concurrencyProfileLineDebug = "#define TRDK_CONCURRENCY_PROFILE_DEBUG (::trdk::Lib::Concurrency::" + concurrencyProfileDebug + ")"
	var concurrencyProfileLineTest = "#define TRDK_CONCURRENCY_PROFILE_TEST (::trdk::Lib::Concurrency::" + concurrencyProfileTest + ")"
	var concurrencyProfileLineRelease = "#define TRDK_CONCURRENCY_PROFILE_RELEASE (::trdk::Lib::Concurrency::" + concurrencyProfileRelease + ")"

	var requiredModulesLine = '';
	for (var i = 0; i < requiredModules.length; ++i) {
		requiredModulesLine += '"' + requiredModules[i] + '", ';
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

	var restrictionsFile = fileSystem.CreateTextFile(outputDir + "Restrictions.h", true);
	restrictionsFile.WriteLine("");
	restrictionsFile.WriteLine("#pragma once");
	restrictionsFile.WriteLine("");
	if (branch != "master" && (guaranteedUsageDaysNumber != 0 || numberOfDaysBeforeUsageStop != 0)) {
		var date = new Date;
		date.setDate(date.getDate() + guaranteedUsageDaysNumber);
		restrictionsFile.WriteLine("// " + date.toString());
		restrictionsFile.WriteLine("#define TRDK_GUARANTEED_USAGE_STOP_TIMESTAMP_MS " + date.getTime());
		date = new Date;
		date.setDate(date.getDate() + numberOfDaysBeforeUsageStop);
		restrictionsFile.WriteLine("// " + date.toString());
		restrictionsFile.WriteLine("#define TRDK_USAGE_STOP_TIMESTAMP_MS " + date.getTime());
	} else {
		restrictionsFile.WriteLine("#define TRDK_GUARANTEED_USAGE_STOP_TIMESTAMP_MS 0");
		restrictionsFile.WriteLine("#define TRDK_USAGE_STOP_TIMESTAMP_MS 0");
	}
	restrictionsFile.WriteLine("");

}

function CreateVersionCmdFile() {
	
	var version = GetVersion();
	if (version == null) {
		return;
	}

	var f = fileSystem.CreateTextFile(outputDir + "SetVersion.cmd", true);
	f.WriteLine("");
	f.WriteLine("set TrdkVersionRelease=" + version.release);
	f.WriteLine("set TrdkVersionBuild=" + version.build);
	f.WriteLine("set TrdkVersionStatus=" + version.status);
	f.WriteLine("set TrdkVersionBranch=" + branch);
	f.WriteLine("");
	f.WriteLine("set TrdkVersion=%TrdkVersionMajorHigh%.%TrdkVersionMajorLow%.%TrdkVersionMinorHigh%");
	f.WriteLine("set TrdkVersionFull=%TrdkVersionMajorHigh%.%TrdkVersionMajorLow%.%TrdkVersionMinorHigh%.%TrdkVersionMinorLow%");
	f.WriteLine("");
	f.WriteLine("set TrdkVendor=" + vendorName);
	f.WriteLine("set TrdkDomain=" + domain);
	f.WriteLine("set TrdkSupportEmail=" + supportEmail);
	f.WriteLine("set TrdkName=" + productName);

}

function main() {
	if (!GetArgs()) {
		shell.Popup("Version compiling: Wrong arguments!");
		return;
	}
	CreateVersionCppHeaderFile();
	CreateVersionCmdFile();
}

//////////////////////////////////////////////////////////////////////////

try {
	main();
} catch (e) {
	shell.Popup("Version compiling: " + e.message);
}

//////////////////////////////////////////////////////////////////////////
