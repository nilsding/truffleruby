@echo off
rem ---------------------------------------------------------------------------
rem jruby.bat - Start Script for the JRuby Interpreter
rem
rem for info on environment variables, see internal batch script _jrubyvars.bat

call "%~dp0_jrubyvars" %*

%_STARTJAVA% %_VM_OPTS% -cp "%CLASSPATH%" -Dfile.encoding=iso88591 -Djruby.base="%JRUBY_BASE%" -Djruby.home="%JRUBY_HOME%" -Djruby.lib="%JRUBY_HOME%\lib" -Djruby.shell="cmd.exe" -Djruby.script=jruby.bat org.jruby.Main %JRUBY_OPTS% %_RUBY_OPTS%
set E=%ERRORLEVEL%

call "%~dp0_jrubycleanup"
