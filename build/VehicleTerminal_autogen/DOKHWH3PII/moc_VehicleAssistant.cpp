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
    QByteArrayData data[31];
    char stringdata0[465];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_VehicleAssistant_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_VehicleAssistant_t qt_meta_stringdata_VehicleAssistant = {
    {
QT_MOC_LITERAL(0, 0, 16), // "VehicleAssistant"
QT_MOC_LITERAL(1, 17, 11), // "answerToken"
QT_MOC_LITERAL(2, 29, 0), // ""
QT_MOC_LITERAL(3, 30, 5), // "token"
QT_MOC_LITERAL(4, 36, 14), // "answerFinished"
QT_MOC_LITERAL(5, 51, 13), // "answerStopped"
QT_MOC_LITERAL(6, 65, 17), // "generatingChanged"
QT_MOC_LITERAL(7, 83, 15), // "stoppingChanged"
QT_MOC_LITERAL(8, 99, 12), // "chatRejected"
QT_MOC_LITERAL(9, 112, 6), // "reason"
QT_MOC_LITERAL(10, 119, 14), // "openCameraPage"
QT_MOC_LITERAL(11, 134, 15), // "openGalleryPage"
QT_MOC_LITERAL(12, 150, 28), // "currentConversationIdChanged"
QT_MOC_LITERAL(13, 179, 22), // "newConversationStarted"
QT_MOC_LITERAL(14, 202, 14), // "conversationId"
QT_MOC_LITERAL(15, 217, 29), // "conversationOperationRejected"
QT_MOC_LITERAL(16, 247, 18), // "conversationLoaded"
QT_MOC_LITERAL(17, 266, 8), // "messages"
QT_MOC_LITERAL(18, 275, 19), // "conversationDeleted"
QT_MOC_LITERAL(19, 295, 10), // "wasCurrent"
QT_MOC_LITERAL(20, 306, 4), // "chat"
QT_MOC_LITERAL(21, 311, 8), // "question"
QT_MOC_LITERAL(22, 320, 14), // "stopGeneration"
QT_MOC_LITERAL(23, 335, 15), // "newConversation"
QT_MOC_LITERAL(24, 351, 19), // "conversationHistory"
QT_MOC_LITERAL(25, 371, 15), // "historyMessages"
QT_MOC_LITERAL(26, 387, 16), // "openConversation"
QT_MOC_LITERAL(27, 404, 18), // "deleteConversation"
QT_MOC_LITERAL(28, 423, 10), // "generating"
QT_MOC_LITERAL(29, 434, 8), // "stopping"
QT_MOC_LITERAL(30, 443, 21) // "currentConversationId"

    },
    "VehicleAssistant\0answerToken\0\0token\0"
    "answerFinished\0answerStopped\0"
    "generatingChanged\0stoppingChanged\0"
    "chatRejected\0reason\0openCameraPage\0"
    "openGalleryPage\0currentConversationIdChanged\0"
    "newConversationStarted\0conversationId\0"
    "conversationOperationRejected\0"
    "conversationLoaded\0messages\0"
    "conversationDeleted\0wasCurrent\0chat\0"
    "question\0stopGeneration\0newConversation\0"
    "conversationHistory\0historyMessages\0"
    "openConversation\0deleteConversation\0"
    "generating\0stopping\0currentConversationId"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_VehicleAssistant[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      20,   14, // methods
       3,  158, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      13,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,  114,    2, 0x06 /* Public */,
       4,    0,  117,    2, 0x06 /* Public */,
       5,    0,  118,    2, 0x06 /* Public */,
       6,    0,  119,    2, 0x06 /* Public */,
       7,    0,  120,    2, 0x06 /* Public */,
       8,    1,  121,    2, 0x06 /* Public */,
      10,    0,  124,    2, 0x06 /* Public */,
      11,    0,  125,    2, 0x06 /* Public */,
      12,    0,  126,    2, 0x06 /* Public */,
      13,    1,  127,    2, 0x06 /* Public */,
      15,    1,  130,    2, 0x06 /* Public */,
      16,    2,  133,    2, 0x06 /* Public */,
      18,    2,  138,    2, 0x06 /* Public */,

 // methods: name, argc, parameters, tag, flags
      20,    1,  143,    2, 0x02 /* Public */,
      22,    0,  146,    2, 0x02 /* Public */,
      23,    0,  147,    2, 0x02 /* Public */,
      24,    0,  148,    2, 0x02 /* Public */,
      25,    1,  149,    2, 0x02 /* Public */,
      26,    1,  152,    2, 0x02 /* Public */,
      27,    1,  155,    2, 0x02 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   14,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void, QMetaType::QString, QMetaType::QVariantList,   14,   17,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,   14,   19,

 // methods: parameters
    QMetaType::Void, QMetaType::QString,   21,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::QVariantList,
    QMetaType::QVariantList, QMetaType::QString,   14,
    QMetaType::Void, QMetaType::QString,   14,
    QMetaType::Void, QMetaType::QString,   14,

 // properties: name, type, flags
      28, QMetaType::Bool, 0x00495001,
      29, QMetaType::Bool, 0x00495001,
      30, QMetaType::QString, 0x00495001,

 // properties: notify_signal_id
       3,
       4,
       8,

       0        // eod
};

void VehicleAssistant::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<VehicleAssistant *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->answerToken((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 1: _t->answerFinished(); break;
        case 2: _t->answerStopped(); break;
        case 3: _t->generatingChanged(); break;
        case 4: _t->stoppingChanged(); break;
        case 5: _t->chatRejected((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 6: _t->openCameraPage(); break;
        case 7: _t->openGalleryPage(); break;
        case 8: _t->currentConversationIdChanged(); break;
        case 9: _t->newConversationStarted((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 10: _t->conversationOperationRejected((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 11: _t->conversationLoaded((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QVariantList(*)>(_a[2]))); break;
        case 12: _t->conversationDeleted((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 13: _t->chat((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 14: _t->stopGeneration(); break;
        case 15: _t->newConversation(); break;
        case 16: { QVariantList _r = _t->conversationHistory();
            if (_a[0]) *reinterpret_cast< QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 17: { QVariantList _r = _t->historyMessages((*reinterpret_cast< const QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 18: _t->openConversation((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 19: _t->deleteConversation((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (VehicleAssistant::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::answerToken)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::answerFinished)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::answerStopped)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::generatingChanged)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::stoppingChanged)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::chatRejected)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::openCameraPage)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::openGalleryPage)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::currentConversationIdChanged)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::newConversationStarted)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::conversationOperationRejected)) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)(QString , QVariantList );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::conversationLoaded)) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (VehicleAssistant::*)(QString , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VehicleAssistant::conversationDeleted)) {
                *result = 12;
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
        if (_id < 20)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 20;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 20)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 20;
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
void VehicleAssistant::answerToken(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void VehicleAssistant::answerFinished()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void VehicleAssistant::answerStopped()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void VehicleAssistant::generatingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void VehicleAssistant::stoppingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void VehicleAssistant::chatRejected(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void VehicleAssistant::openCameraPage()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void VehicleAssistant::openGalleryPage()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void VehicleAssistant::currentConversationIdChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void VehicleAssistant::newConversationStarted(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void VehicleAssistant::conversationOperationRejected(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void VehicleAssistant::conversationLoaded(QString _t1, QVariantList _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void VehicleAssistant::conversationDeleted(QString _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
