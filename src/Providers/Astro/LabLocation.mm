//
//  LabLocation.mm
//  LabExcelsior
//
//  Created by Nick Porcino on 3/29/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

// this ObjC class is a CLLocationManagerDelegate
// it is used to get the current location
@interface LocationDelegate : NSObject <CLLocationManagerDelegate>
@end

@implementation LocationDelegate {
    CLLocationManager *locationManager;
}

// init
- (id) init {
    self = [super init];
    if (!self) return nil;

    // Create a location manager
    locationManager = [[CLLocationManager alloc] init];
    locationManager.delegate = self;
    locationManager.desiredAccuracy = kCLLocationAccuracyBest;
    [locationManager requestWhenInUseAuthorization];
    return self;
}

- (void) locationManagerDidChangeAuthorization:(CLLocationManager *)manager {
    printf("Location services changed authorization\n");

    auto status = [CLLocationManager authorizationStatus];
    switch (status) {
        case kCLAuthorizationStatusNotDetermined:
            printf("Authorization status not determined\n");
            break;
        case kCLAuthorizationStatusRestricted:
            printf("Authorization status restricted\n");
            break;
        case kCLAuthorizationStatusDenied:
            printf("Authorization status denied\n");
            break;
        case kCLAuthorizationStatusAuthorizedAlways:
            printf("Authorization status authorized always\n");
            break;
#if IPHONEOS
        case kCLAuthorizationStatusAuthorizedWhenInUse:
            printf("Authorization status authorized when in use\n");
            break;
#endif
        default:
            printf("Authorization status unknown\n");
            break;
    }
#if IPHONEOS
    bool authorized = status == kCLAuthorizationStatusAuthorizedAlways || status == kCLAuthorizationStatusAuthorizedWhenInUse;
#else
    bool authorized = status == kCLAuthorizationStatusAuthorizedAlways;
#endif
    if (authorized) {
        [locationManager startUpdatingLocation];
    }
    else {
        printf("Location services not authorized\n");
        [locationManager stopUpdatingLocation];
    }
}

- (void) locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray *)locations {
    CLLocation *location = [locations lastObject];
    if (location != nil) {
        // Extract latitude and longitude
        double lat = location.coordinate.latitude;
        double lon = location.coordinate.longitude;
        printf("Latitude: %f\n", lat);
        printf("Longitude: %f\n", lon);
    }
}

- (void) didFailWithError:(NSError *)error {
    printf("Location services failed with error: %s\n", [[error localizedDescription] UTF8String]);
    [locationManager stopUpdatingLocation];
}

- (void) getLatitude:(double*)latitude longitude:(double*)longitude {
    // Get the current location
    CLLocation *currentLocation = locationManager.location;
    if (currentLocation != nil) {
        // Extract latitude and longitude
        *latitude = currentLocation.coordinate.latitude;
        *longitude = currentLocation.coordinate.longitude;
    } else {
        printf("Unable to retrieve location. Defaulting to San Francisco\n");
        *latitude = 37.7749; // near San Francisco
        *longitude = -122.4194; // near San Francisco
    }
    // Stop updating location
    [locationManager stopUpdatingLocation];
}

@end


namespace lab {

static LocationDelegate *locationDelegate = nil;

bool GetLocation(float& latitude, float& longitude) {
    bool ret = false;
    if (!locationDelegate) {
        locationDelegate = [[LocationDelegate alloc] init];
    }

    double lat, lon;
    [locationDelegate getLatitude:&lat longitude:&lon];
    return ret;
}

tm LocalTimeFromUTC(tm utc) {
    // Create a date components object
    NSDateComponents *components = [[NSDateComponents alloc] init];
    [components setYear:utc.tm_year + 1900];
    [components setMonth:utc.tm_mon + 1];
    [components setDay:utc.tm_mday];
    [components setHour:utc.tm_hour];
    [components setMinute:utc.tm_min];
    [components setSecond:utc.tm_sec];

    // Create a calendar object
    NSCalendar *calendar = [NSCalendar currentCalendar];

    // Set the time zone to UTC
    [calendar setTimeZone:[NSTimeZone timeZoneWithAbbreviation:@"UTC"]];

    // Get the date from components
    NSDate *utcDate = [calendar dateFromComponents:components];

    // Convert UTC date to local time
    NSTimeZone *localTimeZone = [NSTimeZone localTimeZone];
    NSInteger localTimeZoneOffset = [localTimeZone secondsFromGMTForDate:utcDate];
    NSDate *localDate = [utcDate dateByAddingTimeInterval:localTimeZoneOffset];

    // get the components of the local date
    NSDateComponents *localComponents = [calendar components:NSCalendarUnitYear | NSCalendarUnitMonth | NSCalendarUnitDay | NSCalendarUnitHour | NSCalendarUnitMinute | NSCalendarUnitSecond fromDate:localDate];
    tm ret;
    ret.tm_year = (int)localComponents.year - 1900; // Years since 1900
    ret.tm_mon = (int)localComponents.month - 1;    // Month (0-11)
    ret.tm_mday = (int)localComponents.day;         // Day of the month (1-31)
    ret.tm_hour = (int)localComponents.hour;        // Hour (0-23)
    ret.tm_min = (int)localComponents.minute;       // Minute (0-59)
    ret.tm_sec = (int)localComponents.second;       // Second (0-59)
    ret.tm_isdst = (int)[localTimeZone isDaylightSavingTimeForDate:localDate] ? 1 : 0;
    return ret;
}


} // lab
