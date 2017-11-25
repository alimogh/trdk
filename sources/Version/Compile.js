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

var branch = "master"
var productName	= "Trading Robot Development Kit"
var vendorName = "Eugene V. Palchukovsky"
var domain = "robotdk.com"
var licenseServiceSubdomain = "licensing"
var supportEmail = "support@" + domain
var copyright = "Copyright 2017 (C) " + vendorName + ", " + domain + ". All rights reserved."
var concurrencyProfileDebug = "PROFILE_RELAX"
var concurrencyProfileTest = "PROFILE_RELAX"
var concurrencyProfileRelease = "PROFILE_RELAX"
var requiredModules = [
	'Core'
	, 'Engine'
	, 'FrontEnd'
	, 'Shell'
	, 'TestTradingSystems'
	, 'InteractiveBrokers'
	, 'FixProtocol'
	, 'Rest'
	, 'Services'
	, 'TestStrategy'
	, 'ArbitrationAdvisor'
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

	var versionReleaseLine		= "#define TRDK_VERSION_RELEASE\t" + version.release;
	var versionBuildLine		= "#define TRDK_VERSION_BUILD\t" + version.build;
	var versionStatusLine		= "#define TRDK_VERSION_STATUS\t" + version.status;

	var versionBranchLine		= "#define TRDK_VERSION_BRANCH\t\t\"" + branch + "\"";
	var versionBranchLineW		= "#define TRDK_VERSION_BRANCH_W\tL\"" + branch + "\"";

	var vendorLine				= "#define TRDK_VENDOR\t\t\"" + vendorName + "\"";
	var vendorLineW				= "#define TRDK_VENDOR_W\tL\"" + vendorName + "\"";

	var domainLine				= "#define TRDK_DOMAIN\t\t\"" + domain + "\"";
	var domainLineW				= "#define TRDK_DOMAIN_W\tL\"" + domain + "\"";

	var supportEmailLine		= "#define TRDK_SUPPORT_EMAIL\t\t\"" + supportEmail + "\"";
	var supportEmailLineW		= "#define TRDK_SUPPORT_EMAIL_W\tL\"" + supportEmail + "\"";

	var licenseServiceSubdomainLine = "#define TRDK_LICENSE_SERVICE_SUBDOMAIN\t\t\"" + licenseServiceSubdomain + "\"";
	var licenseServiceSubdomainLineW = "#define TRDK_LICENSE_SERVICE_SUBDOMAIN_W\tL\"" + licenseServiceSubdomain + "\"";
	
	var nameLine				= "#define TRDK_NAME\t\"" + productName + "\"";
	var nameLineW				= "#define TRDK_NAME_W\tL\"" + productName + "\"";
	
	var copyrightLine			= "#define TRDK_COPYRIGHT\t\t\"" + copyright + "\"";
	var copyrightLineW = "#define TRDK_COPYRIGHT_W\tL\"" + copyright + "\"";

	var concurrencyProfileLineDebug = "#define TRDK_CONCURRENCY_PROFILE_DEBUG (::trdk::Lib::Concurrency::" + concurrencyProfileDebug + ")"
	var concurrencyProfileLineTest = "#define TRDK_CONCURRENCY_PROFILE_TEST (::trdk::Lib::Concurrency::" + concurrencyProfileTest + ")"
	var concurrencyProfileLineRelease = "#define TRDK_CONCURRENCY_PROFILE_RELEASE (::trdk::Lib::Concurrency::" + concurrencyProfileRelease + ")"

	var requiredModulesLine = '';
	for (var i = 0; i < requiredModules.length; ++i) {
		requiredModulesLine += '"' + requiredModules[i] + '", ';
	}
	requiredModulesLine = '#define TRDK_GET_REQUIRED_MODUE_FILE_NAME_LIST() {' + requiredModulesLine + '};';

	var fullFileName = outputDir + "Version.h";
	if (
			IsExistsInFile(fullFileName, versionReleaseLine)
			&& IsExistsInFile(fullFileName, versionBuildLine)
			&& IsExistsInFile(fullFileName, versionStatusLine)
			&& IsExistsInFile(fullFileName, versionBranchLine)
			&& IsExistsInFile(fullFileName, vendorLine)
			&& IsExistsInFile(fullFileName, domainLine)
			&& IsExistsInFile(fullFileName, supportEmailLine)
			&& IsExistsInFile(fullFileName, licenseServiceSubdomainLine)
			&& IsExistsInFile(fullFileName, nameLine)
			&& IsExistsInFile(fullFileName, copyrightLine)
			&& IsExistsInFile(fullFileName, concurrencyProfileLineDebug)
			&& IsExistsInFile(fullFileName, concurrencyProfileLineTest)
			&& IsExistsInFile(fullFileName, concurrencyProfileLineRelease)
			&& IsExistsInFile(fullFileName, requiredModulesLine)) {
		return;
	}

	var f = fileSystem.CreateTextFile(fullFileName, true);
	f.WriteLine("");
	f.WriteLine("#pragma once");
	f.WriteLine("");
	f.WriteLine(versionReleaseLine);
	f.WriteLine(versionBuildLine);
	f.WriteLine(versionStatusLine);
	f.WriteLine("");
	f.WriteLine(versionBranchLine);
	f.WriteLine(versionBranchLineW);
	f.WriteLine("");
	f.WriteLine(vendorLine);
	f.WriteLine(vendorLineW);
	f.WriteLine("");
	f.WriteLine(domainLine);
	f.WriteLine(domainLineW);
	f.WriteLine("");
	f.WriteLine(supportEmailLine);
	f.WriteLine(supportEmailLineW);
	f.WriteLine("");
	f.WriteLine(licenseServiceSubdomainLine);
	f.WriteLine(licenseServiceSubdomainLineW);
	f.WriteLine("");
	f.WriteLine(nameLine);
	f.WriteLine(nameLineW);
	f.WriteLine("");
	f.WriteLine(copyrightLine);
	f.WriteLine(copyrightLineW);
	f.WriteLine("");
	f.WriteLine(concurrencyProfileLineDebug);
	f.WriteLine(concurrencyProfileLineTest);
	f.WriteLine(concurrencyProfileLineRelease);
	f.WriteLine("");
	f.WriteLine(requiredModulesLine);
	f.WriteLine("");

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
