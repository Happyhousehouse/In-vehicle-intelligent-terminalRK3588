/****************************************************************************
** Meta object code from reading C++ file 'OnboardController.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../controller/OnboardController.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'OnboardController.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_OnboardController_t {
    QByteArrayData data[12];
    char stringdata0[189];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_OnboardController_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_OnboardController_t qt_meta_stringdata_OnboardController = {
    {
QT_MOC_LITERAL(0, 0, 17), // "OnboardController"
QT_MOC_LITERAL(1, 18, 22), // "keyboardVisibleChanged"
QT_MOC_LITERAL(2, 41, 0), // ""
QT_MOC_LITERAL(3, 42, 19), // "onPropertiesChanged"
QT_MOC_LITERAL(4, 62, 13), // "interfaceName"
QT_MOC_LITERAL(5, 76, 17), // "changedProperties"
QT_MOC_LITERAL(6, 94, 21), // "invalidatedProperties"
QT_MOC_LITERAL(7, 116, 12), // "showKeyboard"
QT_MOC_LITERAL(8, 129, 12), // "hideKeyboard"
QT_MOC_LITERAL(9, 142, 12), // "quitKeyboard"
QT_MOC_LITERAL(10, 155, 17), // "toggleInputMethod"
QT_MOC_LITERAL(11, 173, 15) // "keyboardVisible"

    },
    "OnboardController\0keyboardVisibleChanged\0"
    "\0onPropertiesChanged\0interfaceName\0"
    "changedProperties\0invalidatedProperties\0"
    "showKeyboard\0hideKeyboard\0quitKeyboard\0"
    "toggleInputMethod\0keyboardVisible"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_OnboardController[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       1,   56, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   44,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       3,    3,   45,    2, 0x08 /* Private */,

 // methods: name, argc, parameters, tag, flags
       7,    0,   52,    2, 0x02 /* Public */,
       8,    0,   53,    2, 0x02 /* Public */,
       9,    0,   54,    2, 0x02 /* Public */,
      10,    0,   55,    2, 0x02 /* Public */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QVariantMap, QMetaType::QStringList,    4,    5,    6,

 // methods: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // properties: name, type, flags
      11, QMetaType::Bool, 0x00495001,

 // properties: notify_signal_id
       0,

       0        // eod
};

void OnboardController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<OnboardController *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->keyboardVisibleChanged(); break;
        case 1: _t->onPropertiesChanged((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QVariantMap(*)>(_a[2])),(*reinterpret_cast< const QStringList(*)>(_a[3]))); break;
        case 2: _t->showKeyboard(); break;
        case 3: _t->hideKeyboard(); break;
        case 4: _t->quitKeyboard(); break;
        case 5: _t->toggleInputMethod(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (OnboardController::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&OnboardController::keyboardVisibleChanged)) {
                *result = 0;
                return;
            }
        }
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<OnboardController *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = _t->keyboardVisible(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
    } else if (_c == QMetaObject::ResetProperty) {
    }
#endif // QT_NO_PROPERTIES
}

QT_INIT_METAOBJECT const QMetaObject OnboardController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_OnboardController.data,
    qt_meta_data_OnboardController,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *OnboardController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *OnboardController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_OnboardController.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int OnboardController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 1;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void OnboardController::keyboardVisibleChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
