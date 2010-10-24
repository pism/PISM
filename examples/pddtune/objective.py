#!/usr/bin/env python
from sys import argv, exit
from getopt import getopt, GetoptError

usage = """
OBJECTIVE.PY  Computes sum and L^2 sums (whole sheet and below 2000 m)
of differences of identified variables.  Puts result in text file.  Specifically,
to evaluate closeness of 'acab' in foo.nc compared to 'smb' in bar.nc, and
put the result as a line in diffs.txt:

  objective.py -v acab,smb -H start.nc foo.nc bar.nc diffs.txt

Here start.nc must contain variable 'thk' to use as ice sheet thickness.
The differences are only computed at locations where thk>1.0, and a separate
sum of squares is computed for the areas with thk<2000.0.

The results are appended to the third argument (file) diffs.txt.  If there is 
no third argument, the results are written to stdout.

Run with --help or --usage to get a this usage message.
"""

def usagefailure(message):
    print message
    print
    print usage
    exit(2)

if __name__ == "__main__":
    try:
      opts, args = getopt(argv[1:], "v:H:", ["help","usage"])
    except GetoptError:
      usagefailure('ERROR: INCORRECT COMMAND LINE ARGUMENTS FOR objective.py')
    dovars = []
    for (opt, optarg) in opts:
        if opt == "-v":
            dovars = optarg.split(",")
        if opt == "-H":
            thkfile = optarg
        if opt in ("--help", "--usage"):
            print usage
            exit(0)

    if (len(args) < 2) | (len(args) > 3):
      usagefailure('ERROR: objective.py requires two or three file names')

    if len(dovars) != 2:
      usagefailure('ERROR: -v option for objective.py requires exactly two variable names')

    try:
      from netCDF4 import Dataset as NC
    except:
      from netCDF3 import Dataset as NC

    try:
      nc_thk = NC(thkfile, 'r')
    except:
      usagefailure("ERROR: FILE '%s' CANNOT BE OPENED FOR READING" % thkfile)

    try:
      nc_pism = NC(args[0], 'r')
    except:
      usagefailure("ERROR: FILE '%s' CANNOT BE OPENED FOR READING" % args[0])

    try:
      nc_reference = NC(args[1], 'r')
    except:
      usagefailure("ERROR: FILE '%s' CANNOT BE OPENED FOR READING" % args[1])

    from numpy import squeeze, shape, double

    print "  comparing variable '%s' in %s to '%s' in %s ..." % \
        (dovars[0], args[0], dovars[1], args[1])

    pism_var = squeeze(nc_pism.variables[dovars[0]][:])
    ref_var = squeeze(nc_reference.variables[dovars[1]][:])
    Mx, My = shape(pism_var)
    
    thk_var = squeeze(nc_thk.variables["thk"][:])

    sumdiff = 0.0
    sumL2 = 0.0
    sum2kL2 = 0.0
    countvalid = 0
    count2k = 0
    for i in range(Mx):
      for j in range(My):
        if (thk_var[i,j] > 1.0):
          countvalid += 1
          diff_vars = pism_var[i,j] - ref_var[i,j]
          sumdiff += diff_vars
          sumL2 += diff_vars * diff_vars
          if (thk_var[i,j] < 2000.0):
            count2k += 1
            sum2kL2 += diff_vars * diff_vars

    avdiff = sumdiff / double(countvalid)
    avL2 = sumL2 / double(countvalid)
    av2kL2 = sum2kL2 / double(count2k)
    
    nc_thk.close()
    nc_pism.close()
    nc_reference.close()

    print "  %d locations for valid (thk>0) comparison:" % countvalid
    print "    average of signed differences (whole sheet) is  %12.7f" % avdiff
    print "    average of squared differences (whole sheet) is %12.7f" % avL2
    print "    average of squared differences (H < 2000) is    %12.7f" % av2kL2

    # write results to file if a file name was given
    if len(args) >= 3:
      print "  writing these values to text file %s ..." % args[2]
      diffs = file(args[2],'a')
      diffs.write("%s %12.7f %12.7f %12.7f\n" % (args[0], avdiff, avL2, av2kL2))
      diffs.close()

