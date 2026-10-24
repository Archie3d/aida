with Ada.Text_IO; use Ada.Text_IO;
with Incomplete_Body;
procedure TaggedIncomplete is
    type Scalar;
    subtype Scalar_View is Scalar;
    type Scalar is range 1 .. 20;
    Scalar_Object : Scalar_View := 7;
    type Unknown (<>);
    type Unknown_Access is access Unknown;
    type Unknown (Kind : Integer) is null record;
    Default_Kind : constant Integer := 4;
    type Known (Kind : Integer := Default_Kind);
    type Known_Access is access Known;
    type Constrained_Access is access Known (4);
    type Known (Kind : Integer := Default_Kind) is null record;
    type Node is tagged;
    type Pointer is access all Node;
    type Class_Pointer is access all Node'Class;
    procedure Set (Object : in out Node; Value : Integer);
    type Node is tagged record
        Value : Integer := 1;
        Next : Pointer;
    end record;
    procedure Set (Object : in out Node; Value : Integer) is
    begin
        Object.Value := Value;
    end Set;
    Unknown_Object : Unknown_Access := new Unknown (3);
    Known_Object : Known_Access := new Known (4);
    Constrained_Object : Constrained_Access := new Known (4);
    Object : aliased Node;
    P : Pointer := Object'Access;
begin
    P.Set (23);
    if Unknown_Object.Kind /= 3 or Known_Object.Kind /= 4 or Constrained_Object.Kind /= 4 then raise Program_Error; end if;
    if Object.Value /= 23 or Scalar_Object /= 7 or Incomplete_Body.Read /= 31 then raise Program_Error; end if;
    Put_Line ("tagged incomplete passed");
end TaggedIncomplete;
