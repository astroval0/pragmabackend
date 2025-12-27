project on pause for now. feel free to contribute via pull request.





















git clone https://github.com/astroval0/pragmabackend
cd pragmabackend
git submodule update --init

once you've compiled, you'll need to add an auth.json to the root dir with this schema:

```json
{
  "steamApiKey": "<YOUR STEAM WEB API KEY HERE https://steamcommunity.com/dev/apikey >"
}
```

if you're on the vivox reimplementation branch, append this to auth.json (ofc fill in your credentials from unity) (only needed if you want to reimplement the voice and text chat):

```json
    "vivox": {
        "server": "",
        "domain": "",
        "issuer": "",
        "key": ""
    }
```

after all that, you will want to pull the launcher (https://github.com/astroval0/SpectreLauncher) and edit common.h; replace the BACKEND_ADDRESS value with L"http://127.0.0.1:8081"
now replace the HOST[] value in page_trigger.cpp with L"127.0.0.1" 

it's best to use Jetbrains CLion IDE to build and run this, use these build settings or else it will not compile.

<img width="1366" height="733" alt="image" src="https://github.com/user-attachments/assets/241ce157-5d1a-4483-be13-dbc68cdad0b3" />


last is to simply just run the backend and run the launcher.




