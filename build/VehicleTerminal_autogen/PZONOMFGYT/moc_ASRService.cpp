/****************************************************************************
** Meta object code from reading C++ file 'ASRService.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../audio/ASRService.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ASRService.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ASRService_t {
    QByteArrayData data[9];
    char stringdata0[90];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ASRService_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ASRService_t qt_meta_stringdata_ASRService = {
    {
QT_MOC_LITERAL(0, 0, 10), // "ASRService"
QT_MOC_LITERAL(1, 11, 18), // "recognitionStarted"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 19), // "recognitionFinished"
QT_MOC_LITERAL(4, 51, 4), // "text"
QT_MOC_LITERAL(5, 56, 13), // "errorOccurred"
QT_MOC_LITERAL(6, 70, 5), // "error"
QT_MOC_LITERAL(7, 76, 9), // "recognize"
QT_MOC_LITERAL(8, 86, 3) // "pcm"

    },
    "ASRService\0recognitionStarted\0\0"
    "recognitionFinished\0text\0errorOccurred\0"
    "error\0recognize\0pcm"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ASRService[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   34,    2, 0x06 /* Public */,
       3,    1,   35,    2, 0x06 /* Public */,
       5,    1,   38,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       7,    1,   41,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::QString,    6,

 // slots: parameters
    QMetaType::Void, QMetaType::QByteArray,    8,

       0        // eod
};

void ASRService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ASRService *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->recognitionStarted(); break;
        case 1: _t->recognitionFinished((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 2: _t->errorOccurred((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 3: _t->recognize((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ASRService::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ASRService::recognitionStarted)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ASRService::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ASRService::recognitionFinished)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ASRService::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ASRService::errorOccurred)) {
                *result = 2;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ASRService::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ASRService.data,
    qt_meta_data_ASRService,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ASRService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ASRService::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ASRService.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ASRService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
    return _id;
}

// SIGNAL 0
void ASRService::recognitionStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ASRService::recognitionFinished(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void ASRService::errorOccurred(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
