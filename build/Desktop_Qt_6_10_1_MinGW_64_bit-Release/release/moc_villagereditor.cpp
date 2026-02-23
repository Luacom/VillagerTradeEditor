/****************************************************************************
** Meta object code from reading C++ file 'villagereditor.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../villagereditor.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'villagereditor.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN14VillagerEditorE_t {};
} // unnamed namespace

template <> constexpr inline auto VillagerEditor::qt_create_metaobjectdata<qt_meta_tag_ZN14VillagerEditorE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "VillagerEditor",
        "openItemConfigEditor",
        "",
        "loadFile",
        "saveFile",
        "onTableItemSelected",
        "row",
        "column",
        "addTradeOption",
        "deleteTradeOption",
        "onDataChanged",
        "onTagCheckboxToggled",
        "openItemSelector",
        "ItemWidgets*",
        "widgets",
        "onGlobalAttributeChanged"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'openItemConfigEditor'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'loadFile'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'saveFile'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onTableItemSelected'
        QtMocHelpers::SlotData<void(int, int)>(5, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 6 }, { QMetaType::Int, 7 },
        }}),
        // Slot 'addTradeOption'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'deleteTradeOption'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onDataChanged'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onTagCheckboxToggled'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'openItemSelector'
        QtMocHelpers::SlotData<void(ItemWidgets *)>(12, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 13, 14 },
        }}),
        // Slot 'onGlobalAttributeChanged'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<VillagerEditor, qt_meta_tag_ZN14VillagerEditorE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject VillagerEditor::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14VillagerEditorE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14VillagerEditorE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14VillagerEditorE_t>.metaTypes,
    nullptr
} };

void VillagerEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<VillagerEditor *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->openItemConfigEditor(); break;
        case 1: _t->loadFile(); break;
        case 2: _t->saveFile(); break;
        case 3: _t->onTableItemSelected((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 4: _t->addTradeOption(); break;
        case 5: _t->deleteTradeOption(); break;
        case 6: _t->onDataChanged(); break;
        case 7: _t->onTagCheckboxToggled(); break;
        case 8: _t->openItemSelector((*reinterpret_cast<std::add_pointer_t<ItemWidgets*>>(_a[1]))); break;
        case 9: _t->onGlobalAttributeChanged(); break;
        default: ;
        }
    }
}

const QMetaObject *VillagerEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *VillagerEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14VillagerEditorE_t>.strings))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int VillagerEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 10;
    }
    return _id;
}
QT_WARNING_POP
