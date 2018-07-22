odb.exe ^
	--generate-query --generate-schema  --generate-session ^
	--database sqlite ^
	--std c++14 ^
	--profile qt --profile boost ^
	-I ../.. -I %BOOST_INCLUDE_DIR% -I %QTDIR%/include ^
		-I ../../../externals/better-enums ^
	-D TRDK_FRONTEND_LIB ^
	--odb-prologue "#include \"OdbPrologue.hpp\"" ^
	--cxx-prologue "#include \"Prec.hpp\"" ^
	--output-dir GeneratedFiles/Orm ^
	 %1