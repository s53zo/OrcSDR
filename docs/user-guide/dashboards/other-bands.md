# AM, WX, CB, and shared receiver routes

The current application has dedicated AM, Shortwave and CB presentation plus a shared
Radio/Scope/Capture receiver surface. Weather and the dashboard-catalog entries
for Airband, Marine, and Satellite route into that shared surface;
they are not separate full decoder applications.

- **Radio** tunes and listens with the mode currently implemented for the route.
- **Scope** shows spectrum and waterfall activity.
- **Capture** records the post-demodulated audio path.

AM uses broadcast-band tuning and filtering. WX uses NFM on a configured NOAA
weather channel. CB exposes its channel-oriented controls. Airband, Marine, and Satellite currently use generic Browse/NFM routing, so this UI does
not establish correct aviation AM voice or a satellite decoder. The older standalone Browse navigation entry is retired; the shared
surface remains the implementation used by these band-entry routes.

Shortwave has a dedicated dashboard with AM plus experimental USB, LSB and CW
audio reception. See [HF modes](../../HF_MODES.md) for controls and validation
limits.
