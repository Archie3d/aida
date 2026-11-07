package body Ada.Numerics.Float_Random is
    package Engine renames Ada.Numerics.Random_Engine;
    function Random (Gen : Generator) return Uniformly_Distributed is
    begin
        return Engine.Random_Float (Gen.Data'Address);
    end Random;
    procedure Reset (Gen : in Generator; Initiator : in Integer) is
    begin
        Engine.Reset (Gen.Data'Address, Initiator);
    end Reset;
    procedure Reset (Gen : in Generator) is
    begin
        Engine.Reset (Gen.Data'Address);
    end Reset;
    procedure Save (Gen : in Generator; To_State : out State) is
    begin
        To_State.Data := Gen.Data;
    end Save;
    procedure Reset (Gen : in Generator; From_State : in State) is
    begin
        Engine.Restore (Gen.Data'Address, From_State.Data'Address);
    end Reset;
    function Image (Of_State : State) return String is
    begin
        return Engine.Image_Of (Of_State.Data);
    end Image;
    function Value (Coded_State : String) return State is
    begin
        return (Data => Engine.Value_Of (Coded_State));
    end Value;
end Ada.Numerics.Float_Random;
