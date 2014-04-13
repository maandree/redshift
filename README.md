This branch contains features not yet merged into the mainline.
Included features not merge into the mainline:

### Improved autoselection of adjustment method
Redshift reads environment variables to figure
out which adjustment method to use if not method
has been selected.

### Fix dayness jump on transition direction switch
If you send a signal to redshift to exit or toggle
while it is in the progress of a transition it will
jump to the end of that transition and then start
the new transition. This bug has been fix.

### Reading output of external commands to get configurations
This feature will probably not be merge into the mainline.

It is possible to use external commands in the configuration
file, with fallback values if the command fails.

For example, changing the line `crtc=crtc-index` in
redshift.conf to
```sh
crtc=fallback-crtc-index$(get-crtc-by-edid long-edid-value)
```
will run the command `sh -c "get-crtc-by-edid long-edid-value"`
and read its output and use that for the `crtc` setting. But if
`get-crtc-by-edid` is killed by a signal or exit with a value
other than zero, or otherwise cannot be executed successfully,
it with use the value `fallback-crtc-index`.

### Fake Windows GDI support using RandR
If built with `--enable-fakegdi --enable-wingdi` it is
possible to use the `wingdi` adjustment method without
Windows. If built with this options, when `wingdi` is
used the Windows GDI calls will be translated to X RandR
calls. This is intended for testing updates to the
`gamma-w32gdi` without have to run Windows.

### Change in the -l and -m arguments
`:` in the argument for the options `-l` and `-m`
has been changed to `,`.

### Multimonitor support on Windows
This as not been tested on Windows.
The option `display=N` has been added to the `wingdi`
method. It is used to select monitor.

### Screen versus CRTC hint
Redshift suggests specifying CRTC if it thinks
you specified screen instead by mistake.

### Preservation of calibrations
The option `-P` or configuration `preserve-calibrations`
can be used if not running in one shot mode. When used,
the current calibrations will not be overridden, instread
they will be applied on top of the adjustments made by
Redshift.

### Concurrent multimonitor and multiscreen support
By specifying adjustment method settings multiple
times in `redshift.conf` as separate sections, you
can use multiple CRTC:s and multiple screens or
graphics cards concurrently. The gamma settings
can be selected specified for the CRTC:s individually.
It is also possible to use `all` instead of an index.

### Multidisplay support
Similar to the previous section, it is possible to
specify multiple X displays. However, it is not
possible to use `display=all`.

### Compile-time configuration limits
It is possible to, at compile to, set
`{MIN,MAX}_{BRIGHTNESS,GAMMA}` and `TRANSITION_{LOW,HIGH}`.
The upper brightness and gamma boundaries can be removed
by defining `NO_MAX_{BRIGHTNESS,GAMMA}`.

### Figures out why Direct Rending Manager is not working
If redshift fails to use DRM it tries to figure out
why and informs the user. If is not able to send a
gamma ramp update it will check why and take
appropriate actions, rather than ignoring it and
assuming that it is because you are in a display
server.

### Extended display identification data support
When using RandR or DRM it is possible to select monitor
by its EDID value. A value used to aid plug-and-play,
and are in but rare cases unique. Namely, thay may,
and most often do, contain the monitor's serial
number or random data, week and year of manufacture,
some other kind of serial number, and prototype
calibration information. Becuase of this addition it
is also possible to use `ignorable=1` which causes
redshift to skip a display, graphics card, screen,
CRTC or monitor it cannot find.

