local base_dir = "./";
local includes_base = {"lib/"}
local sq_sources = [==[
globals.c
am.c
solve.c
help.c
parse.c
cmds.c
simplify.c
factor.c
super.c
unfactor.c
poly.c
diff.c
integrate.c
complex.c
complex_lib.c
list.c
gcd.c
factor_int.c
main.c
]==];

local included = {};
local inc_sys = {};
local inc_sys_count = 0;
local out = io.stdout

function CopyWithInline(prefix, filename)
	if included[filename] then return end
	included[filename] = true
	print('//--Start of', filename);
	--if(filename:match("luac?.c"))
	local inp = io.open(prefix .. filename, "r")
	if not inp then
		for idx in ipairs(includes_base) do
			local sdir = includes_base[idx]
			local fn = prefix .. sdir .. filename
			--print(fn)
			inp = io.open(fn, "r")
			if inp then break end
		end
	end
	if not inp then
		if filename == "fzn_picat_sat_bc.h" then
			print('//--End of', filename);
		end
	else
		assert(inp)
		for line in inp:lines() do
			if line:match('#define LUA_USE_READLINE') then
				out:write("//" .. line .. "\n")
			else
				local inc = line:match('#include%s+(["<].-)[">]')
				if inc  then
					out:write("//" .. line .. "\n")
					if inc:sub(1,1) == '"' or inc:match('[<"]sq') then
						CopyWithInline(prefix, inc:sub(2))
					else
						local fn = inc:sub(2)
						if inc_sys[fn] == null then
							inc_sys_count = inc_sys_count +1
							inc_sys[fn] = inc_sys_count
						end
					end
				else
					out:write(line .. "\n")
				end
			end
		end
		print('//--End of', filename);
	end
end

print([==[
#ifdef WITH_COSMOPOLITAN

STATIC_STACK_SIZE(0x400000);

#endif

#ifndef __COSMOPOLITAN__
//gcc -Wall -DUNIX -DVERSION=\"16.0.5\"  -o mathomatic mathomatic-am.c -lm
#include <sys/resource.h> //7
#include <readline/history.h> //21
#include <unistd.h> //3
#include <limits.h> //8
#include <setjmp.h> //11
#include <getopt.h> //27
//#include <wincon.h> //26
#include <readline/readline.h> //20
#include <math.h> //10
#include <editline.h> //22
#include <string.h> //13
//#include <windows.h> //25
#include <float.h> //9
#include <libgen.h> //4
#include <termios.h> //24
#include <libintl.h> //16
#include <term.h> //19
#include <curses.h> //18
#include <locale.h> //17
#include <ctype.h> //12
#include <stdlib.h> //2
#include <sys/ioctl.h> //23
#include <errno.h> //14
#include <signal.h> //15
#include <stdio.h> //1
//#include <ieeefp.h> //5
#include <sys/time.h> //6

#endif
]==])

local prefix = base_dir; local src_files = sq_sources;
for filename in src_files:gmatch('([^\n]+)') do
	CopyWithInline(prefix, filename);
end
--for k, v in pairs(inc_sys) do print("#include <" .. k .. "> //" .. v ) end
