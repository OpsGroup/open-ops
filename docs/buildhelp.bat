@echo off
:: Generation of Reprise Help
..\deps\win32\bin\doxygen ..\source\Reprise\Doxyfile
cd Reprise\html
..\..\..\deps\win32\bin\hhc index.hhp
cd ..\..

:: Generation of OPS_Core Help
..\deps\win32\bin\doxygen ..\source\OPS_Core\Doxyfile
cd OPS_Core\html
..\..\..\deps\win32\bin\hhc index.hhp
cd ..\..