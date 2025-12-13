/*
 * Copyright (C) 2020-2022, Jan Grulich
 * Copyright (C) 2023, Neal Gompa
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 3.0 of the
 * License.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#pragma once

#include <libportal/portal.h>

#include <QMap>
#include <QStringList>
#include <QSharedPointer>
#include <QVariant>
#include <QWindow>

XDP_PUBLIC
XdpParent *xdp_parent_new_qt (QWindow *window);

namespace XdpQt {

// Returns a global instance of XdpPortal object and takes care
// of its deletion
XDP_PUBLIC
XdpPortal *globalPortalObject();

// Account portal helpers
struct GetUserInformationResult {
    QString id;
    QString name;
    QString image;
};

XDP_PUBLIC
GetUserInformationResult getUserInformationResultFromGVariant(GVariant *variant);

// FileChooser portal helpers
enum FileChooserFilterRuleType{
    Pattern = 0,
    Mimetype = 1
};

struct FileChooserFilterRule {
    FileChooserFilterRuleType type;
    QString rule;
};

struct FileChooserFilter {
    QString label;
    QList<FileChooserFilterRule> rules;
};

struct FileChooserChoice {
    QString id;
    QString label;
    QMap<QString, QString> options;
    QString selected;
};

XDP_PUBLIC
GVariant *filechooserFilesToGVariant(const QStringList &files);

XDP_PUBLIC
GVariant *filechooserFilterToGVariant(const FileChooserFilter &filter);

XDP_PUBLIC
GVariant *filechooserFiltersToGVariant(const QList<FileChooserFilter> &filters);

XDP_PUBLIC
GVariant *filechooserChoicesToGVariant(const QList<FileChooserChoice> &choices);

struct FileChooserResult {
    QMap<QString, QString> choices;
    QStringList uris;
};

XDP_PUBLIC
FileChooserResult filechooserResultFromGVariant(GVariant *variant);

// Notification portal helpers
struct NotificationButton {
    QString label;
    QString action;
    QVariant target;
};

struct Notification {
    QString title;
    QString body;
    QString icon;
    QPixmap pixmap;
    QString priority;
    QString defaultAction;
    QVariant defaultTarget;
    QList<NotificationButton> buttons;
};

XDP_PUBLIC
GVariant *notificationToGVariant(const Notification &notification);

XDP_PUBLIC
QVariant GVariantToQVariant(GVariant *variant);

} // namespace XdpQt
