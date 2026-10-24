with Ada.Text_IO;
with Unchecked_Deallocation;
procedure UnitRenaming is
    package IO renames Ada.Text_IO;
    Elaborations : Integer := 0;
    package Original is
        Value : Integer := 4;
        Failure : exception;
        type Item is private;
        function Get (Object : Item) return Integer;
        package Inner is
            Number : Integer := 7;
        end Inner;
    private
        type Item is record
            Data : Integer := 42;
        end record;
    end Original;
    package body Original is
        function Get (Object : Item) return Integer is
        begin
            return Object.Data;
        end Get;
    begin
        Elaborations := Elaborations + 1;
    end Original;
    package First renames Original;
    package Second renames First;
    package Inner_View renames Second.Inner;
    use First, Second;
    Object : First.Item;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
    Seed : Integer := 13;
    generic
        Amount : Integer := 3;
    package Template is
        Value : Integer := Seed + Amount;
    end Template;
    generic package Template_View renames Template;
    generic package Template_Again renames Template_View;
    package Instance is new Template_Again;
    package Instance_View renames Instance;
    generic
        Increment : Integer := 2;
    function Add (Value : Integer) return Integer;
    function Add (Value : Integer) return Integer is
    begin
        return Seed + Value + Increment;
    end Add;
    generic function Add_View renames Add;
    generic function Add_Again renames Add_View;
    function Calculate is new Add_Again;
    generic
        Amount : Integer := 5;
    procedure Bump (Target : in out Integer);
    procedure Bump (Target : in out Integer) is
    begin
        Target := Target + Amount;
    end Bump;
    generic procedure Bump_View renames Bump;
    procedure Update is new Bump_View (Amount => 6);
    generic
    package Wrapper is
        package View renames Original;
        generic function Function_View renames Add_View;
        generic
        function Local_Source return Integer;
        generic function Local_View renames Local_Source;
    end Wrapper;
    package body Wrapper is
        function Local_Source return Integer is
        begin
            return 23;
        end Local_Source;
    end Wrapper;
    package Wrapped is new Wrapper;
    function Wrapped_Add is new Wrapped.Function_View (Increment => 4);
    function Local_Generated is new Wrapped.Local_View;
    type Link is access Integer;
    Pointer : Link := new Integer'(9);
    generic procedure Free_View renames Unchecked_Deallocation;
    procedure Free is new Free_View (Integer, Link);
    type Text_Link is access String;
    Text : Text_Link := new String'("owned text");
    procedure Free_Text is new Free_View (String, Text_Link);
begin
    Check (Elaborations = 1);
    Second.Value := 10;
    Update (First.Value);
    Check (Original.Value = 16);
    Check (Get (Object) = 42);
    Check (Inner_View.Number = 7);
    Check (Wrapped.View.Value = 16);
    Check (Instance_View.Value = 16);
    Check (Calculate (1) = 16);
    Check (Wrapped_Add (1) = 18);
    Check (Local_Generated = 23);
    begin
        raise Second.Failure;
    exception
        when Original.Failure => null;
    end;
    declare
        Seed : Integer := 100;
        generic function Local_View renames Add_Again;
        function Local_Add is new Local_View (Increment => 1);
    begin
        Check (Local_Add (1) = 15);
    end;
    -- A renamed generic retains the original body's elaboration requirement.
    begin
        declare
            generic
            function Late return Integer;
            generic function Late_View renames Late;
            function Too_Early is new Late_View;
            function Late return Integer is
            begin
                return 11;
            end Late;
        begin
            raise Storage_Error;
        end;
    exception
        when Program_Error => null;
    end;
    Free (Pointer);
    Check (Pointer = null);
    Free_Text (Text);
    Check (Text = null);
    IO.Put_Line ("package and generic renaming passed");
end UnitRenaming;
