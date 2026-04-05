* ability to set a flag to ignore signal assertions so people can use a libevsig-using library and just SIGNAL_ALL handle at the edge
* include/lib should be renamed to include/evsig so that #include works the same
for users of this library and internal to this library
