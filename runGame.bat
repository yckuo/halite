g++ -std=c++11 MyBot.cpp -o MyBot.exe
g++ -std=c++11 PrevBot.cpp -o PrevBot.exe
.\halite.exe -d "40 40" "MyBot.exe" "PrevBot.exe"
taskkill /F /FI "IMAGENAME eq MyBot.exe"
taskkill /F /FI "IMAGENAME eq PrevBot.exe"
