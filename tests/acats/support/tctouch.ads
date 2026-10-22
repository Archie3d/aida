-- Local adapter for the assertions and finalization traces in selected cases.
package TCTouch is
    procedure Assert (Condition : Boolean; Description : String);
    procedure Assert_Not (Condition : Boolean; Description : String);
    procedure Touch (Tag : Character);
    procedure Validate (Expected : String; Description : String);
end TCTouch;
