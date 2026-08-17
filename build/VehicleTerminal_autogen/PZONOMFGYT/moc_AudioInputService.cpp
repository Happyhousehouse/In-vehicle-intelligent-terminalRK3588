/****************************************************************************
** Meta object code from reading C++ file 'AudioInputService.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../audio/AudioInputService.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AudioInputService.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_AudioInputService_t {
    QByteArrayData data[13];
    char stringdata0[122];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_AudioInputService_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_AudioInputService_t qt_meta_stringdata_AudioInputService = {
    {
QT_MOC_LITERAL(0, 0, 17), // "AudioInputService"
QT_MOC_LITERAL(1, 18, 14), // "runningChanged"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 12), // "levelChanged"
QT_MOC_LITERAL(4, 47, 12), // "pcmDataReady"
QT_MOC_LITERAL(5, 60, 3), // "pcm"
QT_MOC_LITERAL(6, 64, 13), // "errorOccurred"
QT_MOC_LITERAL(7, 78, 5), // "error"
QT_MOC_LITERAL(8, 84, 5), // "start"
QT_MOC_LITERAL(9, 90, 4), // "stop"
QT_MOC_LITERAL(10, 95, 7), // "running"
QT_MOC_LITERAL(11, 103, 9), // "peakLevel"
QT_MOC_LITERAL(12, 113, 8) // "rmsLevel"

    },
    "AudioInputService\0runningChanged\0\0"
    "levelChanged\0pcmDataReady\0pcm\0"
    "errorOccurred\0error\0start\0stop\0running\0"
    "peakLevel\0rmsLevel"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_AudioInputService[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       3,   54, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   44,    2, 0x06 /* Public */,
       3,    0,   45,    2, 0x06 /* Public */,
       4,    1,   46,    2, 0x06 /* Public */,
       6,    1,   49,    2, 0x06 /* Public */,

 // methods: name, argc, parameters, tag, flags
       8,    0,   52,    2, 0x02 /* Public */,
       9,    0,   53,    2, 0x02 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QByteArray,    5,
    QMetaType::Void, QMetaType::QString,    7,

 // methods: parameters
    QMetaType::Bool,
    QMetaType::Void,

 // properties: name, type, flags
      10, QMetaType::Bool, 0x00495001,
      11, QMetaType::Double, 0x00495001,
      12, QMetaType::Double, 0x00495001,

 // properties: notify_signal_id
       0,
       1,
       1,

       0        // eod
};

void AudioInputService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AudioInputService *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->runningChanged(); break;
        case 1: _t->levelChanged(); break;
        case 2: _t->pcmDataReady((*reinterpret_cast< QByteArray(*)>(_a[1]))); break;
        case 3: _t->errorOccurred((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 4: { bool _r = _t->start();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 5: _t->stop(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (AudioInputService::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AudioInputService::runningChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (AudioInputService::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AudioInputService::levelChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (AudioInputService::*)(QByteArray );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AudioInputService::pcmDataReady)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (AudioInputService::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AudioInputService::errorOccurred)) {
                *result = 3;
                return;
            }
        }
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<AudioInputService *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = _t->isRunning(); break;
        case 1: *reinterpret_cast< double*>(_v) = _t->peakLevel(); break;
        case 2: *reinterpret_cast< double*>(_v) = _t->rmsLevel(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
    } else if (_c == QMetaObject::ResetProperty) {
    }
#endif // QT_NO_PROPERTIES
}

QT_INIT_METAOBJECT const QMetaObject AudioInputService::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_AudioInputService.data,
    qt_meta_data_AudioInputService,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *AudioInputService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AudioInputService::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_AudioInputService.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int AudioInputService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 6;
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 3;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void AudioInputService::runningChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void AudioInputService::levelChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void AudioInputService::pcmDataReady(QByteArray _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void AudioInputService::errorOccurred(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
