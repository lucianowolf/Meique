$MEIQUE .. || fail "Early fail :-/"

# WTF, Quote hell here you are.
echo "exe:addCustomFlags(\"-DMSG=\\\\\\\"Changed\\\\\\\"\")" >> ../meique.lua

$MEIQUE || fail "Early fail"

if [ "`./exe`" != "Changed" ]
then
    fail "main.cpp compilation not triggered after compiler command line modification!"
fi
