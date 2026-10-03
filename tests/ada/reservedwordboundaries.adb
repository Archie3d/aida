with Ada.Text_IO; use Ada.Text_IO;

procedure ReservedWordBoundaries is
    -- aliased protected requeue until interface synchronized remain comments.
    Aliased_Value : Integer := 1;
    Protected_Value : Integer := 2;
    Requeue_Value : Integer := 3;
    Until_Value : Integer := 4;
    Interface_Value : Integer := 5;
    Synchronized_Value : Integer := 6;
    Unaliased : Integer := 7;
    Unprotected : Integer := 8;
    Requeueing : Integer := 9;
    Until2 : Integer := 10;
    Interfaces : Integer := 11;
    Unsynchronized : Integer := 12;
begin
    Put_Line ("aliased protected requeue until interface synchronized");
    Put_Line (Integer'Image (Aliased_Value + Protected_Value + Requeue_Value
        + Until_Value + Interface_Value + Synchronized_Value + Unaliased
        + Unprotected + Requeueing + Until2 + Interfaces + Unsynchronized));
end ReservedWordBoundaries;
