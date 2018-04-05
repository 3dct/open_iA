# Script for the automatic creation of test configurations
# Checks the modules in the modules directory specified in the constants
# section below (typically "modules"), and creates several
# test configurations:
#   - one with all modules enabled
#   - one with no modules enabled
#   - one for each module with only the module (plus its dependencies) enabled
# In a second step, a script is created to run all those test configurations.

import os, re, sys

if (len(sys.argv) != 6 ):
	print("Invalid number of arguments")
	print("Expected Syntax:")
	print("  $ CreateTestConfiguration.py <SrcDir> <BranchName> <ConfigOutFolder> <ModuleDirs (separate multiple dirs by +)>")
	sys.exit(1)

SrcDir = sys.argv[1]
GitBranchName = sys.argv[2]
ConfigOutFolder = sys.argv[3]
ModuleDirs = sys.argv[4]
BuildNameBase = sys.argv[5]

# Constants:
ModuleDirList = ModuleDirs.split("+")
RunnerScriptLinux = SrcDir + '/Test_files/TestRunner.sh'
RunnerScriptWindows = SrcDir + '/Test_files/TestRunner.bat'

AllModulesOnScript = 'all_flags.cmake'
AllModulesOffScript = 'no_flags.cmake'

print("script for creating open_iA module compilation cmake scripts")

moduleNamesByDir = dict()
moduleNames = []
for dir in ModuleDirList:
	moduleNamesList = next(os.walk(dir))[1]
	moduleNamesByDir[dir] = moduleNamesList
	moduleNames.extend(moduleNamesList)

# write cmake file for enabling all modules:
with open(ConfigOutFolder+'/'+AllModulesOnScript, 'w') as file:
	file.write('SET (SITE "${FIX_SITE}" CACHE STRING "" FORCE)\n')
	file.write('SET (BUILDNAME "'+BuildNameBase+'-'+GitBranchName+'-AllModules" CACHE STRING "" FORCE)\n')
	for dirname in moduleNames:
		file.write('SET (Module_'+dirname+' "ON" CACHE BOOL "" FORCE)\n')

# write cmake file for disabling all modules:
with open(ConfigOutFolder+'/'+AllModulesOffScript, 'w') as file:
	file.write('SET (SITE "${FIX_SITE}" CACHE STRING "" FORCE)\n')
	file.write('SET (BUILDNAME "'+BuildNameBase+'-'+GitBranchName+'-NoModules" CACHE STRING "" FORCE)\n')
	for dirname in moduleNames:
		file.write('SET (Module_'+dirname+' "OFF" CACHE BOOL "" FORCE)\n')

# determine dependencies for each module
dependencies = dict()
for dir in ModuleDirList:
	curDirModules = moduleNamesByDir[dir]
	for module in curDirModules:
		#print(module)
		DepFileName = dir+'/'+module+'/Dependencies.cmake'
		if os.path.isfile(DepFileName):
			with open(DepFileName) as depfile:
				data = depfile.read()
			m = re.search(r"DEPENDENCIES_MODULES\s+([^)]+)", data, re.MULTILINE)
			if m:
				dependencies[module] = m.group(1).split()

# recursively resolve dependencies:
recursiveDeps = dict()
for key in dependencies:
	depstack = list(dependencies[key])
	recursiveDeps[key] = list()
	while (depstack):
		subdep = depstack.pop()
		if (subdep == key):
			print(key+": Circular dependency")
		if (subdep != key and not subdep in recursiveDeps[key]):
			recursiveDeps[key].append(subdep)
			if (subdep in dependencies):
				depstack.extend(dependencies[subdep])
			elif (subdep not in moduleNames):
				print("In module "+key+": unknown dependency '"+subdep+"'")

#print("Resolved dependencies:")
#for key in recursiveDeps:
#	print(key+": ")
#	print(recursiveDeps[key])

# write one build config per module in first given module directory:
firstDirModuleNames = moduleNamesByDir[ModuleDirList[0]]
for module in firstDirModuleNames:
	cmakeFileName = ConfigOutFolder+'/Module_'+module+'.cmake'
	with open(cmakeFileName, 'w') as file:
		file.write('SET (SITE "${FIX_SITE}" CACHE STRING "" FORCE)\n')
		file.write('SET (BUILDNAME "'+BuildNameBase+'-'+GitBranchName+'-'+module+'" CACHE STRING "" FORCE)\n')
		file.write('SET (Module_'+module+' "ON" CACHE BOOL "" FORCE)\n')
		if module in recursiveDeps:
			file.write('\n')
			for submodule in recursiveDeps[module]:
				file.write('SET (Module_'+submodule+' "ON" CACHE BOOL "" FORCE)\n')

print("Done.")
