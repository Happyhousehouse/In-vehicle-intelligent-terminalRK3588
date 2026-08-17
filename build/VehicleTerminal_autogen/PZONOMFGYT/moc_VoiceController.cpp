/****************************************************************************
** Meta object code from reading C++ file 'VoiceController.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../audio/VoiceController.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'VoiceController.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_VoiceController_t {
    QByteArrayData data[15];
    char stringdata0[191];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_VoiceController_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_VoiceController_t qt_meta_stringdata_VoiceController = {
    {
QT_MOC_LITERAL(0, 0, 15), // "VoiceController"
QT_MOC_LITERAL(1, 16, 18), // "onWakeWordDetected"
QT_MOC_LITERAL(2, 35, 0), // ""
QT_MOC_LITERAL(3, 36, 7), // "keyword"
QT_MOC_LITERAL(4, 44, 15), // "onSpeechStarted"
QT_MOC_LITERAL(5, 60, 20), // "onSpeechSegmentReady"
QT_MOC_LITERAL(6, 81, 3), // "pcm"
QT_MOC_LITERAL(7, 85, 21), // "onRecognitionFinished"
QT_MOC_LITERAL(8, 107, 4), // "text"
QT_MOC_LITERAL(9, 112, 10), // "onAsrError"
QT_MOC_LITERAL(10, 123, 5), // "error"
QT_MOC_LITERAL(11, 129, 25), // "onAssistantAnswerFinished"
QT_MOC_LITERAL(12, 155, 14), // "onChatRejected"
QT_MOC_LITERAL(13, 170, 6), // "reason"
QT_MOC_LITERAL(14, 177, 13) // "onWaitTimeout"

    },
    "VoiceController\0onWakeWordDetected\0\0"
    "keyword\0onSpeechStarted\0onSpeechSegmentReady\0"
    "pcm\0onRecognitionFinished\0text\0"
    "onAsrError\0error\0onAssistantAnswerFinished\0"
    "onChatRejected\0reason\0onWaitTimeout"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_VoiceController[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   54,    2, 0x08 /* Private */,
       4,    0,   57,    2, 0x08 /* Private */,
       5,    1,   58,    2, 0x08 /* Private */,
       7,    1,   61,    2, 0x08 /* Private */,
       9,    1,   64,    2, 0x08 /* Private */,
      11,    0,   67,    2, 0x08 /* Private */,
      12,    1,   68,    2, 0x08 /* Private */,
      14,    0,   71,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QByteArray,    6,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, QMetaType::QString,   10,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   13,
    QMetaType::Void,

       0        // eod
};

void VoiceController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<VoiceController *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onWakeWordDetected((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->onSpeechStarted(); break;
        case 2: _t->onSpeechSegmentReady((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 3: _t->onRecognitionFinished((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->onAsrError((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->onAssistantAnswerFinished(); break;
        case 6: _t->onChatRejected((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->onWaitTimeout(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject VoiceController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_VoiceController.data,
    qt_meta_data_VoiceController,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *VoiceController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *VoiceController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_VoiceController.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int VoiceController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 8;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
