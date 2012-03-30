AddOption('--cutil-lib',
		  dest='cutil_lib',
		  type='string',
		  nargs=1,
		  action='store',
		  metavar='DIR',
		  default='/usr/local/lib',
		  help='directory with libcutil.a')
AddOption('--cutil-include',
		  dest='cutil_include',
		  type='string',
		  nargs=1,
		  action='store',
		  metavar='DIR',
		  default='/usr/local/include',
		  help='directory with cutil headers')
CUTIL_LIB = GetOption('cutil_lib')
CUTIL_INCLUDE = GetOption('cutil_include')
env = Environment(CUTIL_LIB, CUTIL_INCLUDE)
SConscript(['src/SConscript','test/SConscript'], env)
