g++ -std=c++11 MyBot.cpp -o MyBot.exe
g++ -std=c++11 PrevBot.cpp -o PrevBot.exe


for /l %%x in (1, 1, 3) do (
    .\halite.exe -d "40 40" "MyBot.exe" "PrevBot.exe" >> output.txt 2>&1
    taskkill /F /FI "IMAGENAME eq MyBot.exe"
    taskkill /F /FI "IMAGENAME eq PrevBot.exe"
    sleep 5


    .\halite.exe -d "30 30" "MyBot.exe" "PrevBot.exe" >> output.txt 2>&1
    taskkill /F /FI "IMAGENAME eq MyBot.exe"
    taskkill /F /FI "IMAGENAME eq PrevBot.exe"
    sleep 5


    .\halite.exe -d "20 20" "MyBot.exe" "PrevBot.exe" >> output.txt 2>&1
    taskkill /F /FI "IMAGENAME eq MyBot.exe"
    taskkill /F /FI "IMAGENAME eq PrevBot.exe"
    sleep 5
)

