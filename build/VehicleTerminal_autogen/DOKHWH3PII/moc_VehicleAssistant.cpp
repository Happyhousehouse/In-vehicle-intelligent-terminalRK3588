/****************************************************************************
** Meta object code from reading C++ file 'VehicleAssistant.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../assistant/VehicleAssistant.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'VehicleAssistant.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_VehicleAssistant_t {
    QByteArrayData data[44];
    char stringdata0[659];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_VehicleAssistant_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_VehicleAssistant_t qt_meta_stringdata_VehicleAssistant = {
    {
QT_MOC_LITERAL(0, 0, 16), // "VehicleAssistant"
QT_MOC_LITERAL(1, 17, 19), // "userMessageAccepted"
QT_MOC_LITERAL(2, 37, 0), // ""
QT_MOC_LITERAL(3, 38, 7), // "message"
QT_MOC_LITERAL(4, 46, 11), // "answerToken"
QT_MOC_LITERAL(5, 58, 5), // "token"
QT_MOC_LITERAL(6, 64, 22), // "answerSpeechChunkReady"
QT_MOC_LITERAL(7, 87, 4), // "text"
QT_MOC_LITERAL(8, 92, 12), // "pauseAfterMs"
QT_MOC_LITERAL(9, 105, 25), // "weatherPageQueryRequested"
QT_MOC_LITERAL(10, 131, 4), // "city"
QT_MOC_LITERAL(11, 136, 14), // "answerFinished"
QT_MOC_LITERAL(12, 151, 13), // "answerStopped"
QT_MOC_LITERAL(13, 165, 17), // "generatingChanged"
QT_MOC_LITERAL(14, 183, 15), // "stoppingChanged"
QT_MOC_LITERAL(15, 199, 12), // "chatRejected"
QT_MOC_LITERAL(16, 212, 6), // "reason"
QT_MOC_LITERAL(17, 219, 14), // "openCameraPage"
QT_MOC_LITERAL(18, 234, 15), // "closeCameraPage"
QT_MOC_LITERAL(19, 250, 15), // "openGalleryPage"
QT_MOC_LITERAL(20, 266, 16), // "closeGalleryPage"
QT_MOC_LITERAL(21, 283, 16), // "returnToMainPage"
QT_MOC_LITERAL(22, 300, 15), // "openWeatherPage"
QT_MOC_LITERAL(23, 316, 28), // "currentConversationIdChanged"
QT_MOC_LITERAL(24, 345, 22), // "newConversationStarted"
QT_MOC_LITERAL(25, 368, 14), // "conversationId"
QT_MOC_LITERAL(26, 383, 29), // "conversationOperationRejected"
QT_MOC_LITERAL(27, 413, 18), // "conversationLoaded"
QT_MOC_LITERAL(28, 432, 8), // "messages"
QT_MOC_LITERAL(29, 441, 19), // "conversationDeleted"
QT_MOC_LITERAL(30, 461, 10), // "wasCurrent"
QT_MOC_LITERAL(31, 472, 4), // "chat"
QT_MOC_LITERAL(32, 477, 8), // "question"
QT_MOC_LITERAL(33, 486, 14), // "stopGeneration"
QT_MOC_LITERAL(34, 501, 15), // "newConversation"
QT_MOC_LITERAL(35, 517, 19), // "conversationHistory"
QT_MOC_LITERAL(36, 537, 15), // "historyMessages"
QT_MOC_LITERAL(37, 553, 16), // "openConversation"
QT_MOC_LITERAL(38, 570, 18), // "deleteConversation"
QT_MOC_LITERAL(39, 589, 20), // "setWeatherPageActive"
QT_MOC_LITERAL(40, 610, 6), // "active"
QT_MOC_LITERAL(41, 617, 10), // "generating"
QT_MOC_LITERAL(42, 628, 8), // "stopping"
QT_MOC_LITERAL(43, 637, 21) // "currentConversationId"

    },
    "VehicleAssistant\0userMessageAccepted\0"
    "\0message\0answerToken\0token\0"
    "answerSpeechChunkReady\0text\0pauseAfterMs\0"
    "weatherPageQueryRequested\0city\0"
    "answerFinished\0answerStopped\0"
    "generatingChanged\0stoppingChanged\0"
    "chatRejected\0reason\0openCameraPage\0"
    "closeCameraPage\0openGalleryPage\0"
    "closeGalleryPage\0returnToMainPage\0"
    "openWeatherPage\0currentConversationIdChanged\0"
    "newConversationStarted\0conversationId\0"
    "conversationOperationRejected\0"
    "conversationLoaded\0messages\0"
    "conversationDeleted\0wasCurrent\0chat\0"
    "question\0stopGeneration\0newConversation\0"
    "conversationHistory\0historyMessages\0"
    "openConversation\0deleteConversation\0"
    "setWeatherPageActive\0active\0generating\0"
    "stopping\0currentConversationId"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_VehicleAssistant[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      28,   14, // methods
       3,  216, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      20,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,  154,    2, 0x06 /* Public */,
       4,    1,  157,    2, 0x06 /* Public */,
       6,    2,  160,    2, 0x06 /* Public */,
       9,    1,  165,    2, 0x06 /* Public */,
      11,    0,  168,    2, 0x06 /* Public */,
      12,    0,  169,    2, 0x06 /* Public */,
      13,    0,  170,    2, 0x06 /* Public */,
      14,    0,  171,    2, 0x06 /* Public */,
      15,    1,  172,    2, 0x06 /* Public */,
      17,    0,  175,    2, 0x06 /* Public */,
      18,    0,  176,    2, 0x06 /* Public */,
      19,    0,  177,    2, 0x06 /* Public */,
      20,    0,  178,    2, 0x06 /* Public */,
      21,    0,  179,    2, 0x06 /* Public */,
      22,    0,  180,    2, 0x06 /* Public */,
      23,    0,  181,    2, 0x06 /* Public */,
      24,    1,  182,    2, 0x06 /* Public */,
      26,    1,  185,    2, 0x06 /* Public */,
      27,    2,  188,    2, 0x06 /* Public */,
      29,    2,  193,    2, 0x06 /* Public */,

 // methods: name, argc, parameters, tag, flags
      31,    1,  198,    2, 0x02 /* Public */,
      33,    0,  201,    2, 0x02 /* Public */,
      34,    0,  202,    2, 0x02 /* Public */,
      35,    0,  203,    2, 0x02 /* Public */,
      36,    1,  204,    2, 0x02 /* Public */,
      37,    1,  207,    2, 0x02 /* Public */,
      38,    1,  210,    2, 0x02 /* Public */,
      39,    1,  213,    2, 0x02 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,    7,    8,
    QMetaType::Void, QMetaType::QString,   10,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   16,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   25,
    QMetaType::Void, QMetaType::QString,   16,
    QMetaType::Void, QMetaType::QString, QMetaType::QVariantList,   25,   28,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,   25,   30,

 // methods: parameters
    QMetaType::Void, QMetaType::QString,   32,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::QVariantList,
    QMetaType::QVariantList, QMetaType::QString,   25,
    QMetaType::Void, QMetaType::QString,   25,
    QMetaType::Void, QMetaType::QString,   25,
    QMetaType::Void, QMetaType::Bool,   40,

 // properties: name, type, flags
      41, QMetaType::Bool, 0x00495001,
      42, QMetaType::Bool, 0x00495001,
      43, QMetaType::QString, 0x00495001,

 // properties: notify_signal_id
       6,
       7,
      15,

       0        // eod
};

void VehicleAssistant::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<VehicleAssistant *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->userMessageAccepted((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 1: _t->answerToken((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 2: _t->answerSpeechChunkReady((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 3: _t->weatherPageQueryRequested((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->answerFinished(); break;
        case 5: _t->answerStopped(); break;
        case 6: _t->generatingChanged(); break;
        case 7: _t->stoppingChanged(); break;
        case 8: _t->chatRejected((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 9: _t->openCameraPage(); break;
        case 10: _t->closeCameraPage(); break;
        case 11: _t->openGalleryPage(); break;
        case 12: _t->closeGalleryPage(); break;
        case 13: _t->returnToMainPage(); break;
        case 14: _t->openWeatherPage(); break;
        case 15: _t->currentConversationIdChanged(); break;
        case 16: _t->newConversationStarted((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 17: _t->conversationOperationRejected((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 18: _t->conversationLoaded((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QVariantList(*)>(_a[2]))); break;
        case 19: _t->conversationDeleted((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 20: _t->chat((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 21: _t->stopGeneration(); break;
        case 22: _t->newConversation(); break;
        case 23: { QVariantList _r = _t->conversationHistory();
            if (_a[0]) *reinterpret_cast< QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 24: { QVariantList _r = _t->historyMessages((*reinterpret_cast< const QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 25: _t->openConversation((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 26: _t->deleteConversation((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 27: _t->setWeatherPageActive((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (VehicleAssistant::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::userMessageAccepted)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::answerToken)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)(QString , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::answerSpeechChunkReady)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::weatherPageQueryRequested)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::answerFinished)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::answerStopped)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::generatingChanged)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::stoppingChanged)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::chatRejected)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::openCameraPage)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::closeCameraPage)) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::openGalleryPage)) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::closeGalleryPage)) {
                *result = 12;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::returnToMainPage)) {
                *result = 13;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::openWeatherPage)) {
                *result = 14;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::currentConversationIdChanged)) {
                *result = 15;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::newConversationStarted)) {
                *result = 16;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::conversationOperationRejected)) {
                *result = 17;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)(QString , QVariantList );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::conversationLoaded)) {
                *result = 18;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)(QString , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::conversationDeleted)) {
                *result = 19;
                return;
            }
        }
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<VehicleAssistant *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = _t->isGenerating(); break;
        case 1: *reinterpret_cast< bool*>(_v) = _t->isStopping(); break;
        case 2: *reinterpret_cast< QString*>(_v) = _t->currentConversationId(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
    } else if (_c == QMetaObject::ResetProperty) {
    }
#endif // QT_NO_PROPERTIES
}

QT_INIT_METAOBJECT const QMetaObject VehicleAssistant::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_VehicleAssistant.data,
    qt_meta_data_VehicleAssistant,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *VehicleAssistant::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *VehicleAssistant::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_VehicleAssistant.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int VehicleAssistant::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 28)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 28;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 28)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 28;
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
void VehicleAssistant::userMessageAccepted(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void VehicleAssistant::answerToken(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void VehicleAssistant::answerSpeechChunkReady(QString _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void VehicleAssistant::weatherPageQueryRequested(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void VehicleAssistant::answerFinished()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void VehicleAssistant::answerStopped()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void VehicleAssistant::generatingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void VehicleAssistant::stoppingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void VehicleAssistant::chatRejected(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void VehicleAssistant::openCameraPage()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void VehicleAssistant::closeCameraPage()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void VehicleAssistant::openGalleryPage()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}

// SIGNAL 12
void VehicleAssistant::closeGalleryPage()
{
    QMetaObject::activate(this, &staticMetaObject, 12, nullptr);
}

// SIGNAL 13
void VehicleAssistant::returnToMainPage()
{
    QMetaObject::activate(this, &staticMetaObject, 13, nullptr);
}

// SIGNAL 14
void VehicleAssistant::openWeatherPage()
{
    QMetaObject::activate(this, &staticMetaObject, 14, nullptr);
}

// SIGNAL 15
void VehicleAssistant::currentConversationIdChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 15, nullptr);
}

// SIGNAL 16
void VehicleAssistant::newConversationStarted(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 16, _a);
}

// SIGNAL 17
void VehicleAssistant::conversationOperationRejected(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 17, _a);
}

// SIGNAL 18
void VehicleAssistant::conversationLoaded(QString _t1, QVariantList _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 18, _a);
}

// SIGNAL 19
void VehicleAssistant::conversationDeleted(QString _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 19, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
