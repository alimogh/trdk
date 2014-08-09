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

var branch = "morganbenoit"
var productName	= "Trading Robot Development Kit"
var vendorName = "Eugene V. Palchukovsky"
var domain = "robotdk.com"
var licenseServiceSubdomain = "licensing"
var supportEmail ="support@" + domain
var copyright = "Copyright 2014 (C) " + vendorName + ", " + domain + ". All rights reserved."

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
		var expression = /^\s*(\d+)\.(\d+)\.(\d+)\.(\d+)\s*$/;
		var match = expression.exec(f.ReadLine());
		if (match) {
			var version = new Object;
			version.majorHigh = match[1];
			version.majorLow = match[2];
			version.minorHigh = match[3];
			version.minorLow = match[4];
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

	var versionMajorHighLine	= "#define TRDK_VERSION_MAJOR_HIGH\t" + version.majorHigh;
	var versionMajorLowLine		= "#define TRDK_VERSION_MAJOR_LOW\t" + version.majorLow;
	var versionMinorHighLine	= "#define TRDK_VERSION_MINOR_HIGH\t" + version.minorHigh;
	var versionMinorLowLine		= "#define TRDK_VERSION_MINOR_LOW\t" + version.minorLow;

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
	var copyrightLineW			= "#define TRDK_COPYRIGHT_W\tL\"" + copyright + "\"";

	var fullFileName = outputDir + "Version.h";
	if (	IsExistsInFile(fullFileName, versionMajorHighLine)
			&& IsExistsInFile(fullFileName, versionMajorLowLine)
			&& IsExistsInFile(fullFileName, versionMinorHighLine)
			&& IsExistsInFile(fullFileName, versionMinorLowLine)
			&& IsExistsInFile(fullFileName, versionBranchLine)
			&& IsExistsInFile(fullFileName, vendorLine)
			&& IsExistsInFile(fullFileName, domainLine)
			&& IsExistsInFile(fullFileName, supportEmailLine)
			&& IsExistsInFile(fullFileName, licenseServiceSubdomainLine)
			&& IsExistsInFile(fullFileName, nameLine)
			&& IsExistsInFile(fullFileName, copyrightLine)) {
		return;
	}

	var f = fileSystem.CreateTextFile(fullFileName, true);
	f.WriteLine("");
	f.WriteLine("#pragma once");
	f.WriteLine("");
	f.WriteLine(versionMajorHighLine);
	f.WriteLine(versionMajorLowLine);
	f.WriteLine(versionMinorHighLine);
	f.WriteLine(versionMinorLowLine);
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

}

function CreateVersionCmdFile() {
	
	var version = GetVersion();
	if (version == null) {
		return;
	}

	var f = fileSystem.CreateTextFile(outputDir + "SetVersion.cmd", true);
	f.WriteLine("");
	f.WriteLine("set TrdkVersionMajorHigh=" + version.majorHigh);
	f.WriteLine("set TrdkVersionMajorLow=" + version.majorLow);
	f.WriteLine("set TrdkVersionMinorHigh=" + version.minorHigh);
	f.WriteLine("set TrdkVersionMinorLow=" + version.minorLow);
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
