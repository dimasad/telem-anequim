TEMPLATE = subdirs

SUBDIRS += src
exists(qtserialport){
    SUBDIRS += qtserialport
    src.depends += qtserialport
}
