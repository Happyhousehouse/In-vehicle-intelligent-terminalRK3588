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
    QByteArrayData data[20];
    char stringdata0[290];
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
QT_MOC_LITERAL(11, 129, 27), // "onAssistantSpeechChunkReady"
QT_MOC_LITERAL(12, 157, 12), // "pauseAfterMs"
QT_MOC_LITERAL(13, 170, 18), // "onTtsSpeechStarted"
QT_MOC_LITERAL(14, 189, 19), // "onTtsSpeechFinished"
QT_MOC_LITERAL(15, 209, 18), // "onTtsSpeechStopped"
QT_MOC_LITERAL(16, 228, 25), // "onAssistantAnswerFinished"
QT_MOC_LITERAL(17, 254, 14), // "onChatRejected"
QT_MOC_LITERAL(18, 269, 6), // "reason"
QT_MOC_LITERAL(19, 276, 13) // "onWaitTimeout"

    },
    "VoiceController\0onWakeWordDetected\0\0"
    "keyword\0onSpeechStarted\0onSpeechSegmentReady\0"
    "pcm\0onRecognitionFinished\0text\0"
    "onAsrError\0error\0onAssistantSpeechChunkReady\0"
    "pauseAfterMs\0onTtsSpeechStarted\0"
    "onTtsSpeechFinished\0onTtsSpeechStopped\0"
    "onAssistantAnswerFinished\0onChatRejected\0"
    "reason\0onWaitTimeout"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_VoiceController[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   74,    2, 0x08 /* Private */,
       4,    0,   77,    2, 0x08 /* Private */,
       5,    1,   78,    2, 0x08 /* Private */,
       7,    1,   81,    2, 0x08 /* Private */,
       9,    1,   84,    2, 0x08 /* Private */,
      11,    2,   87,    2, 0x08 /* Private */,
      13,    0,   92,    2, 0x08 /* Private */,
      14,    0,   93,    2, 0x08 /* Private */,
      15,    0,   94,    2, 0x08 /* Private */,
      16,    0,   95,    2, 0x08 /* Private */,
      17,    1,   96,    2, 0x08 /* Private */,
      19,    0,   99,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QByteArray,    6,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, QMetaType::QString,   10,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,    8,   12,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   18,
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
        case 5: _t->onAssistantSpeechChunkReady((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 6: _t->onTtsSpeechStarted(); break;
        case 7: _t->onTtsSpeechFinished(); break;
        case 8: _t->onTtsSpeechStopped(); break;
        case 9: _t->onAssistantAnswerFinished(); break;
        case 10: _t->onChatRejected((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 11: _t->onWaitTimeout(); break;
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
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 12;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
