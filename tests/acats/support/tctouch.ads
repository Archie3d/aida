-- Local adapter for the assertions and finalization traces in selected cases.
package TCTouch is
    procedure Assert (Condition : Boolean; Description : String);
    procedure Assert_Not (Condition : Boolean; Description : String);
    procedure Touch (Tag : Character);
    procedure Flush;
    procedure Validate (Expected : String; Message : String;
                        Order_Meaningful : Boolean := True);
    procedure Validate_One_Of
        (Expected_1, Expected_2 : String;
         Expected_3, Expected_4, Expected_5, Expected_6 : String := "";
         Message : String);
end TCTouch;
