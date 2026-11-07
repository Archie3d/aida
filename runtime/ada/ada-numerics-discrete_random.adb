package body Ada.Numerics.Discrete_Random is
    package Engine renames Ada.Numerics.Random_Engine;
    use type Engine.Word;
    Signed_Positions : constant Boolean := Result_Subtype'Pos (Result_Subtype'Base'First) < 0;
    function Random (Gen : Generator) return Result_Subtype is
        Raw : constant Engine.Word := Engine.Random_Discrete (Gen.Data'Address,
            Engine.Word'Mod (Result_Subtype'Pos (Result_Subtype'First)),
            Engine.Word'Mod (Result_Subtype'Pos (Result_Subtype'Last)), not Signed_Positions);
    begin
        if Signed_Positions and Raw > Engine.Word (Long_Integer'Last) then
            return Result_Subtype'Val (-1 - Long_Integer (Engine.Word'Last - Raw));
        end if;
        return Result_Subtype'Val (Raw);
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
begin
    if Result_Subtype'First > Result_Subtype'Last then
        raise Constraint_Error;
    end if;
end Ada.Numerics.Discrete_Random;
