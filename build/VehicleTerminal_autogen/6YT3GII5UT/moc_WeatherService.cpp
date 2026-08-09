/****************************************************************************
** Meta object code from reading C++ file 'WeatherService.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../weather/WeatherService.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'WeatherService.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_WeatherService_t {
    QByteArrayData data[30];
    char stringdata0[390];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_WeatherService_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_WeatherService_t qt_meta_stringdata_WeatherService = {
    {
QT_MOC_LITERAL(0, 0, 14), // "WeatherService"
QT_MOC_LITERAL(1, 15, 14), // "weatherChanged"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 14), // "loadingChanged"
QT_MOC_LITERAL(4, 46, 12), // "errorChanged"
QT_MOC_LITERAL(5, 59, 14), // "updateFinished"
QT_MOC_LITERAL(6, 74, 12), // "updateFailed"
QT_MOC_LITERAL(7, 87, 5), // "error"
QT_MOC_LITERAL(8, 93, 20), // "citySearchingChanged"
QT_MOC_LITERAL(9, 114, 16), // "citySearchFailed"
QT_MOC_LITERAL(10, 131, 19), // "citySearchSucceeded"
QT_MOC_LITERAL(11, 151, 4), // "city"
QT_MOC_LITERAL(12, 156, 22), // "onWeatherReplyFinished"
QT_MOC_LITERAL(13, 179, 14), // "QNetworkReply*"
QT_MOC_LITERAL(14, 194, 5), // "reply"
QT_MOC_LITERAL(15, 200, 24), // "onGeocodingReplyFinished"
QT_MOC_LITERAL(16, 225, 7), // "refresh"
QT_MOC_LITERAL(17, 233, 10), // "searchCity"
QT_MOC_LITERAL(18, 244, 8), // "cityName"
QT_MOC_LITERAL(19, 253, 13), // "citySearching"
QT_MOC_LITERAL(20, 267, 11), // "temperature"
QT_MOC_LITERAL(21, 279, 19), // "apparentTemperature"
QT_MOC_LITERAL(22, 299, 8), // "humidity"
QT_MOC_LITERAL(23, 308, 9), // "windSpeed"
QT_MOC_LITERAL(24, 318, 13), // "precipitation"
QT_MOC_LITERAL(25, 332, 11), // "weatherCode"
QT_MOC_LITERAL(26, 344, 11), // "weatherText"
QT_MOC_LITERAL(27, 356, 7), // "loading"
QT_MOC_LITERAL(28, 364, 11), // "errorString"
QT_MOC_LITERAL(29, 376, 13) // "dailyForecast"

    },
    "WeatherService\0weatherChanged\0\0"
    "loadingChanged\0errorChanged\0updateFinished\0"
    "updateFailed\0error\0citySearchingChanged\0"
    "citySearchFailed\0citySearchSucceeded\0"
    "city\0onWeatherReplyFinished\0QNetworkReply*\0"
    "reply\0onGeocodingReplyFinished\0refresh\0"
    "searchCity\0cityName\0citySearching\0"
    "temperature\0apparentTemperature\0"
    "humidity\0windSpeed\0precipitation\0"
    "weatherCode\0weatherText\0loading\0"
    "errorString\0dailyForecast"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_WeatherService[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   14, // methods
      12,   98, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       8,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   74,    2, 0x06 /* Public */,
       3,    0,   75,    2, 0x06 /* Public */,
       4,    0,   76,    2, 0x06 /* Public */,
       5,    0,   77,    2, 0x06 /* Public */,
       6,    1,   78,    2, 0x06 /* Public */,
       8,    0,   81,    2, 0x06 /* Public */,
       9,    1,   82,    2, 0x06 /* Public */,
      10,    1,   85,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      12,    1,   88,    2, 0x08 /* Private */,
      15,    1,   91,    2, 0x08 /* Private */,

 // methods: name, argc, parameters, tag, flags
      16,    0,   94,    2, 0x02 /* Public */,
      17,    1,   95,    2, 0x02 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, QMetaType::QString,   11,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 13,   14,
    QMetaType::Void, 0x80000000 | 13,   14,

 // methods: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   18,

 // properties: name, type, flags
      11, QMetaType::QString, 0x00495001,
      19, QMetaType::Bool, 0x00495001,
      20, QMetaType::Double, 0x00495001,
      21, QMetaType::Double, 0x00495001,
      22, QMetaType::Int, 0x00495001,
      23, QMetaType::Double, 0x00495001,
      24, QMetaType::Double, 0x00495001,
      25, QMetaType::Int, 0x00495001,
      26, QMetaType::QString, 0x00495001,
      27, QMetaType::Bool, 0x00495001,
      28, QMetaType::QString, 0x00495001,
      29, QMetaType::QVariantList, 0x00495001,

 // properties: notify_signal_id
       0,
       5,
       0,
       0,
       0,
       0,
       0,
       0,
       0,
       1,
       2,
       0,

       0        // eod
};

void WeatherService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<WeatherService *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->weatherChanged(); break;
        case 1: _t->loadingChanged(); break;
        case 2: _t->errorChanged(); break;
        case 3: _t->updateFinished(); break;
        case 4: _t->updateFailed((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 5: _t->citySearchingChanged(); break;
        case 6: _t->citySearchFailed((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 7: _t->citySearchSucceeded((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 8: _t->onWeatherReplyFinished((*reinterpret_cast< QNetworkReply*(*)>(_a[1]))); break;
        case 9: _t->onGeocodingReplyFinished((*reinterpret_cast< QNetworkReply*(*)>(_a[1]))); break;
        case 10: _t->refresh(); break;
        case 11: _t->searchCity((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (WeatherService::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&WeatherService::weatherChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (WeatherService::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&WeatherService::loadingChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (WeatherService::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&WeatherService::errorChanged)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (WeatherService::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&WeatherService::updateFinished)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (WeatherService::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&WeatherService::updateFailed)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (WeatherService::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&WeatherService::citySearchingChanged)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (WeatherService::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&WeatherService::citySearchFailed)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (WeatherService::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&WeatherService::citySearchSucceeded)) {
                *result = 7;
                return;
            }
        }
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<WeatherService *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->city(); break;
        case 1: *reinterpret_cast< bool*>(_v) = _t->citySearching(); break;
        case 2: *reinterpret_cast< double*>(_v) = _t->temperature(); break;
        case 3: *reinterpret_cast< double*>(_v) = _t->apparentTemperature(); break;
        case 4: *reinterpret_cast< int*>(_v) = _t->humidity(); break;
        case 5: *reinterpret_cast< double*>(_v) = _t->windSpeed(); break;
        case 6: *reinterpret_cast< double*>(_v) = _t->precipitation(); break;
        case 7: *reinterpret_cast< int*>(_v) = _t->weatherCode(); break;
        case 8: *reinterpret_cast< QString*>(_v) = _t->weatherText(); break;
        case 9: *reinterpret_cast< bool*>(_v) = _t->loading(); break;
        case 10: *reinterpret_cast< QString*>(_v) = _t->errorString(); break;
        case 11: *reinterpret_cast< QVariantList*>(_v) = _t->dailyForecast(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
    } else if (_c == QMetaObject::ResetProperty) {
    }
#endif // QT_NO_PROPERTIES
}

QT_INIT_METAOBJECT const QMetaObject WeatherService::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_WeatherService.data,
    qt_meta_data_WeatherService,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *WeatherService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *WeatherService::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_WeatherService.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int WeatherService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 12;
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 12;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 12;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 12;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 12;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 12;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void WeatherService::weatherChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void WeatherService::loadingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void WeatherService::errorChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void WeatherService::updateFinished()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void WeatherService::updateFailed(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void WeatherService::citySearchingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void WeatherService::citySearchFailed(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void WeatherService::citySearchSucceeded(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
