/****************************************************************************
** Meta object code from reading C++ file 'FatigueService.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../fatigue/FatigueService.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'FatigueService.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_FatigueService_t {
    QByteArrayData data[10];
    char stringdata0[162];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_FatigueService_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_FatigueService_t qt_meta_stringdata_FatigueService = {
    {
QT_MOC_LITERAL(0, 0, 14), // "FatigueService"
QT_MOC_LITERAL(1, 15, 24), // "monitoringEnabledChanged"
QT_MOC_LITERAL(2, 40, 0), // ""
QT_MOC_LITERAL(3, 41, 23), // "eyeWarningActiveChanged"
QT_MOC_LITERAL(4, 65, 19), // "fatigueLevelChanged"
QT_MOC_LITERAL(5, 85, 20), // "setMonitoringEnabled"
QT_MOC_LITERAL(6, 106, 7), // "enabled"
QT_MOC_LITERAL(7, 114, 17), // "monitoringEnabled"
QT_MOC_LITERAL(8, 132, 12), // "fatigueLevel"
QT_MOC_LITERAL(9, 145, 16) // "eyeWarningActive"

    },
    "FatigueService\0monitoringEnabledChanged\0"
    "\0eyeWarningActiveChanged\0fatigueLevelChanged\0"
    "setMonitoringEnabled\0enabled\0"
    "monitoringEnabled\0fatigueLevel\0"
    "eyeWarningActive"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_FatigueService[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       3,   40, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   34,    2, 0x06 /* Public */,
       3,    0,   35,    2, 0x06 /* Public */,
       4,    0,   36,    2, 0x06 /* Public */,

 // methods: name, argc, parameters, tag, flags
       5,    1,   37,    2, 0x02 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // methods: parameters
    QMetaType::Bool, QMetaType::Bool,    6,

 // properties: name, type, flags
       7, QMetaType::Bool, 0x00495001,
       8, QMetaType::QString, 0x00495001,
       9, QMetaType::Bool, 0x00495001,

 // properties: notify_signal_id
       0,
       2,
       1,

       0        // eod
};

void FatigueService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<FatigueService *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->monitoringEnabledChanged(); break;
        case 1: _t->eyeWarningActiveChanged(); break;
        case 2: _t->fatigueLevelChanged(); break;
        case 3: { bool _r = _t->setMonitoringEnabled((*reinterpret_cast< bool(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (FatigueService::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FatigueService::monitoringEnabledChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (FatigueService::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FatigueService::eyeWarningActiveChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (FatigueService::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FatigueService::fatigueLevelChanged)) {
                *result = 2;
                return;
            }
        }
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<FatigueService *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = _t->isRunning(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->fatigueLevel(); break;
        case 2: *reinterpret_cast< bool*>(_v) = _t->eyeWarningActive(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
    } else if (_c == QMetaObject::ResetProperty) {
    }
#endif // QT_NO_PROPERTIES
}

QT_INIT_METAOBJECT const QMetaObject FatigueService::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_FatigueService.data,
    qt_meta_data_FatigueService,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *FatigueService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FatigueService::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_FatigueService.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int FatigueService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 4;
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
void FatigueService::monitoringEnabledChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void FatigueService::eyeWarningActiveChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void FatigueService::fatigueLevelChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
