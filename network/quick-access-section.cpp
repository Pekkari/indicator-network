/*
 * Copyright (C) 2014 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *     Antti Kaijanmäki <antti.kaijanmaki@canonical.com>
 */

#include "quick-access-section.h"

#include "menuitems/switch-item.h"

#include "menumodel-cpp/action-group-merger.h"
#include "menumodel-cpp/menu-merger.h"

namespace networking = connectivity::networking;

class QuickAccessSection::Private
{
public:
    ActionGroupMerger::Ptr m_actionGroupMerger;
    Menu::Ptr m_menu;

    std::shared_ptr<networking::Manager> m_manager;

    SwitchItem::Ptr m_flightModeSwitch;

    Private(std::shared_ptr<networking::Manager> manager);
};

QuickAccessSection::Private::Private(std::shared_ptr<networking::Manager> manager)
    : m_manager{manager}
{
    m_actionGroupMerger = std::make_shared<ActionGroupMerger>();
    m_menu = std::make_shared<Menu>();

    m_flightModeSwitch = std::make_shared<SwitchItem>(_("Flight Mode"), "airplane", "enabled");
    switch (m_manager->flightMode().get()) {
    case networking::Manager::FlightModeStatus::off:
        m_flightModeSwitch->state().set(false);
        break;
    case networking::Manager::FlightModeStatus::on:
        m_flightModeSwitch->state().set(true);
        break;
    }
    m_manager->flightMode().changed().connect([this](networking::Manager::FlightModeStatus value){
        switch (value) {
        case networking::Manager::FlightModeStatus::off:
            m_flightModeSwitch->state().set(false);
            break;
        case networking::Manager::FlightModeStatus::on:
            m_flightModeSwitch->state().set(true);
            break;
        }
    });


    m_actionGroupMerger->add(*m_flightModeSwitch);
    m_menu->append(*m_flightModeSwitch);
}

QuickAccessSection::QuickAccessSection(std::shared_ptr<networking::Manager> manager)
{
    d.reset(new Private(manager));
}

QuickAccessSection::~QuickAccessSection()
{

}

ActionGroup::Ptr
QuickAccessSection::actionGroup()
{
    return *d->m_actionGroupMerger;
}

MenuModel::Ptr
QuickAccessSection::menuModel()
{
    return d->m_menu;
}
