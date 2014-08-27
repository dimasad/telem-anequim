TEMPLATE = subdirs

SUBDIRS += src tests
exists(qtserialport){
    SUBDIRS += qtserialport
    src.depends += qtserialport
}
