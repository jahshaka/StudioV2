/**************************************************************************
This file is part of JahshakaVR, VR Authoring Toolkit
http://www.jahshaka.com
Copyright (c) 2016  GPLv3 Jahshaka LLC <coders@jahshaka.com>

This is free software: you may copy, redistribute
and/or modify it under the terms of the GPLv3 License

For more information see the LICENSE file
*************************************************************************/

#ifndef PROJECT_H
#define PROJECT_H

#include <QString>
#include <QDateTime>

class Project
{
public:
    QString folderPath;
    QString projectName;
	QString projectFolderGuid;
    QString guid;

    bool _saved;
public:
    Project();
    void setProjectPath(const QString&, const QString &);
    void setProjectGuid(const QString&);
    bool isSaved();

    QString getProjectName();
    QString getProjectFolder();
    QString getProjectGuid();

    static Project *createNew();

    static const QStringList ModelTypesAsString;
};

struct ProjectTileData {
    QString     name;
    QByteArray  thumbnail;
    QString     guid;
};

enum AssetViewFilter : int
{
	Editor = 1,
	AssetsView,
	Effects,
	DontShow // Use for dependencies and hidden types later on
};

struct DatabaseMetadataRecord
{
	QDateTime dateCreated;
	QString hash;
	QString version;
	QByteArray data;

	// specific version
	int major;
	int minor;
	int patch;
};

struct AssetRecord
{
    QString     guid;
	int			type;
	QString     name;
	int			collection;
	int			timesUsed;
    QString     projectGuid;
    QDateTime   dateCreated;
    QDateTime   lastUpdated;
	QString		author;
	QString		license;
	QString		hash;
	QString		version;
	QString		parent;
	QByteArray  thumbnail;
	QByteArray  asset;
	QByteArray  tags;
	QByteArray  properties;
	int			view_filter;
};

struct DependencyRecord
{
    int         dependerType;
    int         dependeeType;
    QString     projectGuid;
    QString     depender;
    QString     dependee;
    QString     id;
};

struct FolderRecord
{
	QString	    guid;
	QString	    name;
	QString	    parent;
    QString     version;
    QString     projectGuid;
    QDateTime   dateCreated;
    QDateTime   lastUpdated;
	int	        count;
    bool        visible;
};

struct CollectionRecord
{
    QString     name;
    QDateTime   dateCreated;
    int         id;
};

enum class ModelTypes
{
    Undefined,		// Used
    Material,		// Supported
    Texture,		// Supported
    Video,
    Sky,			// Supported
    Object,			// Supported
    Mesh,			// Supported
    SoundEffect,
    Music,			// Supported
    Shader,			// Supported
    Variant,
    File,			// Supported
    ParticleSystem	// Supported
};

#define	MODEL_GUID_ROLE		0x0113
#define MODEL_PARENT_ROLE	0x0128
#define	MODEL_EXT_ROLE		0x0133
#define	MODEL_TYPE_ROLE		0x0123
#define	MODEL_MESH_ROLE		0x0173
#define SKY_TYPE_ROLE		0x0179

#define MODEL_ITEM_TYPE		0x0981
#define MODEL_FOLDER		0x0871
#define MODEL_ASSET			0x0421

#define MODEL_GRAPH			0x0321

#endif // PROJECT_H
