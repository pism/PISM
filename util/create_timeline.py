#!/usr/bin/env python

## @package create_timeline
# \author Andy Aschwanden, University of Alaska Fairbanks, USA
# \brief Script creates a timeline for enviornmental forcing.
# \details Script creates a timeline file that can be used with
# the -time_file option and -*_given_file to control the applicaton
# of forcing data, and to control start and end date of a PISM simulation.
#
# Say you have monthly climate forcing from 1980-1-1 through 2001-1-1 in
# the forcing file foo_1980-1999.nc to be used with, e.g. -surface_given_file,
# but you want the model to run from 1991-1-1 through 2001-1-1.
#
# Usage:
#
# \verbatim $ create_timeline.py --start_date '1991-1-1' -end_date '2001-1-1'
# time_1991-2000.nc \endverbatim

from argparse import ArgumentParser
from dateutil import rrule
from dateutil.parser import parse
from datetime import datetime
import time
import numpy as np

try:
    import netCDF4 as netCDF
except:
    import netCDF3 as netCDF
NC = netCDF.Dataset
from netcdftime import utime

# Set up the option parser
parser = ArgumentParser()
parser.description = '''Script creates a time file with time and time
bounds that can be used to determine to force PISM via command line
option -time_file'''
parser.add_argument("FILE", nargs='*')
parser.add_argument("-p", "--periodicity", dest="periodicity",
                    help='''periodicity, e.g. monthly, daily, etc. Default=monthly''',
                    default="monthly")
parser.add_argument("-a", "--start_date", dest="start_date",
                    help='''Start date in ISO format. Default=1989-1-1''',
                    default='1989-1-1')
parser.add_argument("-e", "--end_date", dest="end_date",
                    help='''End date in ISO format. Default=2012-1-1''',
                    default='2012-1-1')
parser.add_argument("-u", "--ref_unit", dest="ref_unit",
                    help='''Reference unit. Default=days. Use of months or
                    years is NOT recommended.''', default='days')
parser.add_argument("-d", "--ref_date", dest="ref_date",
                    help='''Reference date. Default=1960-1-1''',
                    default='1960-1-1')

options = parser.parse_args()
periodicity = options.periodicity.upper()
start_date = parse(options.start_date)
end_date = parse(options.end_date)
ref_unit = options.ref_unit
ref_date = options.ref_date
args = options.FILE
infile = args[0]
nc = NC(infile, 'w', format='NETCDF3_CLASSIC')

time_units = ("%s since %s" % (ref_unit, ref_date))
# currently PISM only supports the gregorian standard calendar
# once this changes, calendar should become a command-line option
time_calendar = "standard"

cdftime = utime(time_units, time_calendar)

# create a dictionary so that we can supply the periodicity as a
# command-line argument.
pdict = {}
pdict['SECONDLY'] = rrule.SECONDLY
pdict['HOURLY'] = rrule.HOURLY
pdict['DAILY'] = rrule.DAILY
pdict['WEEKLY'] = rrule.WEEKLY
pdict['MONTHLY'] = rrule.MONTHLY
pdict['YEARLY'] = rrule.YEARLY
prule = pdict[periodicity]

r = time_units.split(' ')[2].split('-')
refdate = datetime(int(r[0]), int(r[1]), int(r[2]))


bnds_datelist = list(rrule.rrule(prule, dtstart=start_date, until=end_date))

# calculate the days since refdate, including refdate
bnds_interval_since_refdate = cdftime.date2num(bnds_datelist)
time_interval_since_refdate = (bnds_interval_since_refdate[0:-1] +
                               np.diff(bnds_interval_since_refdate) / 2)
# set a fill value
fill_value = np.nan

# create a new dimension for bounds only if it does not yet exist
dim = "time"
if dim not in nc.dimensions.keys():
    nc.createDimension(dim)

# create a new dimension for bounds only if it does not yet exist
dim = "tbnds"
if dim not in nc.dimensions.keys():
    nc.createDimension(dim, 2)

time_var_name = "time"
bnds_var_name = "time_bounds"

time_var = nc.createVariable(time_var_name, 'f', dimensions=("time"))
time_var[:] = time_interval_since_refdate
time_var.bounds = bnds_var_name
time_var.units = time_units
time_var.calendar = time_calendar
time_var.standard_name = time_var_name
time_var.axis = "T"

time_bnds_var = nc.createVariable(bnds_var_name, 'f', dimensions=("time", "tbnds"))
time_bnds_var[:,0] = bnds_interval_since_refdate[0:-1]
time_bnds_var[:,1] = bnds_interval_since_refdate[1::]

# writing global attributes
script_command = ' '.join([time.ctime(), ':', __file__.split('/')[-1],
                           ' '.join([str(x) for x in args])])
nc.history = script_command
nc.Conventions = "CF 1.5"
nc.close()
