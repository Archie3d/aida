-- Local adapter for the assertions and finalization traces in selected cases.
package TCTouch is
    procedure Assert (Condition : Boolean; Description : String);
    procedure Assert_Not (Condition : Boolean; Description : String);
    procedure Touch (Tag : Character);
    procedure Validate (Expected : String; Message : String);
    procedure Validate_One_Of
        (Expected_1, Expected_2, Expected_3, Expected_4, Expected_5, Expected_6 : String;
         Message : String);
end TCTouch;
