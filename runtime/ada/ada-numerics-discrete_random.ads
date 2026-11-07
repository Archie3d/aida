with Ada.Finalization;
with Ada.Numerics.Random_Engine;
generic
    type Result_Subtype is (<>);
package Ada.Numerics.Discrete_Random is
    type Generator is limited private;
    function Random (Gen : Generator) return Result_Subtype;
    procedure Reset (Gen : in Generator; Initiator : in Integer);
    procedure Reset (Gen : in Generator);
    type State is private;
    procedure Save (Gen : in Generator; To_State : out State);
    procedure Reset (Gen : in Generator; From_State : in State);
    Max_Image_Width : constant := Ada.Numerics.Random_Engine.Image_Width;
    function Image (Of_State : State) return String;
    function Value (Coded_State : String) return State;
private
    type Generator is new Ada.Finalization.Limited_Controlled with record
        Data : Ada.Numerics.Random_Engine.State_Words := Ada.Numerics.Random_Engine.Default_State;
    end record;
    type State is record
        Data : Ada.Numerics.Random_Engine.State_Words := Ada.Numerics.Random_Engine.Default_State;
    end record;
end Ada.Numerics.Discrete_Random;
