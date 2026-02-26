FILE_CONTENT=$(cat "./webpage/index.html")
rm webpage.h
touch webpage.h

echo "char MAIN_page[] PROGMEM = R\"=====(" > webpage.h
echo $FILE_CONTENT >> webpage.h
echo ")=====\";" >> webpage.h