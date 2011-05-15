/*
Copyright 2010  Christian Vetter veaac.fdirct@gmail.com

This file is part of MoNav.

MoNav is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

MoNav is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with MoNav.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "interfaces/irouter.h"
#include "mapdata.h"
#include "routinglogic.h"
#include "logger.h"

#include "tripinfodialog.h"
#include "ui_tripinfodialog.h"

#define DIALOGUPDATEINTERVAL 3


TripinfoDialog::TripinfoDialog( QWidget* parent ) :
		QWidget( parent ),
		m_ui( new Ui::TripinfoDialog )
{
	m_ui->setupUi( this );
	QStringList itemNames;
	itemNames << tr( "Ground Speed:" );
	itemNames << tr( "Max. Speed:" );
	itemNames << tr( "Average Speed:" );
	itemNames << tr( "Remaining Distance:" );
	itemNames << tr( "Track Length:" );
	itemNames << tr( "Departure:" );
	itemNames << tr( "Track Time:" );
	itemNames << tr( "Remaining Time:" );
	itemNames << tr( "Arrival:" );
	itemNames << tr( "Max. Elevation:" );
	itemNames << tr( "Min. Elevation:" );
	for( int i = 0; i < itemNames.size(); i++){
		m_listItems.append( new QTreeWidgetItem(0) );
	}
	for( int i = 0; i < m_listItems.size(); i++){
		m_listItems[i]->setText( 0, itemNames[i] );
		m_listItems[i]->setText( 1, "-" );
	}
	m_ui->treeWidget->setColumnCount( 2 );
	m_ui->treeWidget->addTopLevelItems( m_listItems );
	m_ui->treeWidget->resizeColumnToContents( 0 );
	m_ui->treeWidget->setHeaderHidden ( true );
	m_lastUpdateTime = QDateTime::currentDateTime().addSecs( - DIALOGUPDATEINTERVAL );
	connect( m_ui->cancel, SIGNAL(clicked()), this, SIGNAL(cancelled()) );
	m_ui->displayTrackProfile->setMinimumSize( 300, 150 );
	updateInformation();
}

TripinfoDialog::~TripinfoDialog()
{
	delete m_ui;
}

void TripinfoDialog::updateInformation()
{
	// Limit the amount of recalculations
	if( m_lastUpdateTime.secsTo( QDateTime::currentDateTime() ) < DIALOGUPDATEINTERVAL ){
		return;
	}
	m_lastUpdateTime = QDateTime::currentDateTime();

	double routeDistance = 0.0;
	QStringList theValues;

	theValues.append( speedString( RoutingLogic::instance()->groundSpeed() ) );
	theValues.append( speedString( Logger::instance()->maxSpeed() ) );
	theValues.append( speedString( Logger::instance()->averageSpeed() ) );
	theValues.append( distanceString( RoutingLogic::instance()->routeDistance() ) );
	theValues.append( distanceString( Logger::instance()->trackDistance() ) );
	theValues.append( dateString( Logger::instance()->trackStartTime() ) );
	theValues.append( timeString( Logger::instance()->trackDuration() ) );
	double averageSpeed = Logger::instance()->averageSpeed();
	double remainingTime = 0.0;
	// Avoid an division by zero
	if( averageSpeed != 0 ){
		remainingTime = ((routeDistance*60)/(averageSpeed*1000));
	}
	theValues.append( timeString( remainingTime ) );

	if( remainingTime < 1 )
		theValues.append( dateString( QDateTime() ) );
	else
		theValues.append( dateString( QDateTime::currentDateTime().addSecs( (int)remainingTime*60 ) ) );

	theValues.append( elevationString( Logger::instance()->trackMaxElevation() ) );
	theValues.append( elevationString( Logger::instance()->trackMinElevation() ) );

	if( m_listItems.size() == theValues.size() ){
		for( int i = 0; i < m_listItems.size(); i++ ){
			m_listItems[i]->setText( 1, theValues[i] );
		}
	}
	// m_ui->treeWidget->resizeColumnToContents( 1 );

	// Drawing the track's height profile
	// m_ui->displayTrackProfile->setMinimumSize( 300, 150 );
	QPixmap pixmap( m_ui->displayTrackProfile->width(), m_ui->displayTrackProfile->height());
	pixmap.fill( QColor( 255, 255, 255, 128 ) );

	const QVector<double>& trackElevations = Logger::instance()->trackElevations();
	double minElevation = Logger::instance()->trackMinElevation();
	double maxElevation = Logger::instance()->trackMaxElevation();
	double metersDelta = maxElevation - minElevation;
	int margin = 5;
	double scaleX = 1.0;
	double scaleY = 1.0;

	// scaleX = double( m_ui->displayTrackProfile->pixmap()->width() - margin-margin ) / double( trackElevations.size() );
	// scaleY = ( m_ui->displayTrackProfile->pixmap()->height() -margin-margin ) / metersDelta;
	scaleX = double( m_ui->displayTrackProfile->width() - margin-margin ) / double( trackElevations.size() );
	scaleY = ( m_ui->displayTrackProfile->height() -margin-margin ) / metersDelta;

	QPolygonF polygon;
	for( int i = 0; i < trackElevations.size(); i++ ){
		polygon << QPointF( (i * scaleX) +margin, (trackElevations.at(i) * scaleY) -margin );
	}

	QPainter painter;
	painter.begin( &pixmap );
	painter.setPen( QColor( 178, 034, 034 ) );
	painter.drawPolyline( polygon );
	painter.end();

	// Ugly hack to flip the image
	QImage mirroredImage = pixmap.toImage().mirrored( false, true );
	pixmap = QPixmap::fromImage( mirroredImage );
	m_ui->displayTrackProfile->setPixmap( pixmap );
}


QString TripinfoDialog::speedString( double speed )
{
	if( speed < 1 )
		return "-";
	QString speedSignature = "km/h";
	if( m_locale.measurementSystem() == QLocale::ImperialSystem ){
		speedSignature = "mph";
		speed *= 0.62;
	}
	return QString::number( speed, 'f', 1 ).append( speedSignature );
}


QString TripinfoDialog::distanceString( double distance )
{
	if( distance < 1 )
		return "-";
	QString distanceSignature = "km";
	if( m_locale.measurementSystem() == QLocale::ImperialSystem ){
		distanceSignature = "mi";
		distance *= 0.62;
	}
	QString distString = QString::number( distance/1000, 'f', 1 );
	if( distString.contains( "nan" ) )
		distString = "-";
	else
		distString.append( distanceSignature );
		return distString;
}


QString TripinfoDialog::dateString( QDateTime date )
{
	if( !date.isValid() )
		return "-";

	return date.toString( "ddd hh:mm" );
}


QString TripinfoDialog::timeString( double time )
{
	// QTime might be a better choice, but can only cope with up to 24 hours, then it flips.
	QString timeString;
	if( time < 1 )
		timeString = "-";
	else if( time < 600 ){
		timeString = QString::number( time/60, 'f', 1 ).prepend( "0" ).append( "min" );
	}
	else if( time < 3600 ){
		timeString = QString::number( time/60, 'f', 0 ).append( "min" );
	}
	else{
		int hours = time / 3600;
		int minutes = (int)time % 3600 / 60;
		timeString = QString::number( hours ).append( ":" );
		if( minutes < 10 )
			timeString.append( "0" );
		timeString.append( QString::number( minutes ).append( "h" ) );
	}
	return timeString;
}


QString TripinfoDialog::elevationString( double elevation )
{
	QString elevationSignature = "m";
	if( m_locale.measurementSystem() == QLocale::ImperialSystem ){
		elevation *= 3.2808399;
		elevationSignature = "ft";
	}
	QString eleString = QString::number( elevation );
	if( eleString.contains( "nan" ) )
		eleString = "-";
	else
		eleString.append( elevationSignature );
	return eleString;
}

