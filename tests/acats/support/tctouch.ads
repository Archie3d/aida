-- Local adapter for the assertions used by C354002.
package TCTouch is
    procedure Assert (Condition : Boolean; Description : String);
    procedure Assert_Not (Condition : Boolean; Description : String);
end TCTouch;
