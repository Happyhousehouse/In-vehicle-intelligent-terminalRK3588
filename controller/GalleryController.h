#pragma once


#include <QObject>

#include <QStringList>


#include "../photo/GalleryService.h"



class GalleryController:public QObject
{

    Q_OBJECT



public:


    explicit GalleryController(

        QObject *parent=nullptr

    );



    Q_INVOKABLE

    QStringList getPhotos();

    Q_INVOKABLE
    bool deletePhoto(
        QString path
    );

    Q_INVOKABLE
    bool deletePhotos(
            QStringList paths
    );

private:


    GalleryService gallery_;


};