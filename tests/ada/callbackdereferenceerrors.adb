procedure CallbackDereferenceErrors is
    type Action is access procedure (Value : Integer);
    type Function_Link is access function return Integer;
    type Float_Action is access procedure (Value : Float);
    type Output_Action is access procedure (Value : out Integer);
    subtype Positive_Value is Integer range 1 .. Integer'Last;
    type Positive_Action is access procedure (Value : Positive_Value);
    A : Action;
    F : Function_Link;
    W : Float_Action := A.all'Access;
    M : Output_Action := A.all'Access;
    S : Positive_Action := A.all'Access;
    P : Action := F.all'Access;
    procedure Escape (Callback : access procedure (Value : Integer)) is
        Named : Action := Callback.all'Access;
        Anonymous : access procedure (Value : Integer) := Callback.all'Access;
    begin
        null;
    end Escape;
    type C_Action is access procedure (Value : Integer);
    pragma Convention (C, C_Action);
    Native : C_Action;
    Wrong_Ada : Action := Native.all'Access;
    Wrong_C : C_Action := A.all'Access;
    protected Counter is
        procedure Add (Value : Integer);
    end Counter;
    protected body Counter is
        procedure Add (Value : Integer) is
        begin
            null;
        end Add;
    end Counter;
    Protected_Action : access protected procedure (Value : Integer) := Counter.Add'Access;
    Wrong_Protected : Action := Protected_Action.all'Access;
    Arguments : Action := A.all'Access (1);
    procedure Deep is
        type Inner_Action is access procedure (Value : Integer);
        type Derived_Action is new Action;
        Inner : Inner_Action;
        Derived : Derived_Action;
        Outer : Action := Inner.all'Access;
        -- A derived access type retains the ancestor's accessibility level.
        Good : Action := Derived.all'Access;
    begin
        null;
    end Deep;
    type Alternative is access procedure (Value : Integer);
    function Choose return Action is
    begin
        return A;
    end Choose;
    function Choose return Alternative is
    begin
        return null;
    end Choose;
    Ambiguous : Action := Choose.all'Access;
    Unchecked : Action := A.all'Unchecked_Access;
    package Views is
        type Hidden is private;
        function Get return Hidden;
    private
        type Hidden is access procedure (Value : Integer);
    end Views;
    package body Views is
        function Get return Hidden is
        begin
            return null;
        end Get;
    end Views;
    Hidden_Value : Views.Hidden := Views.Get;
    Hidden_Source : Action := Hidden_Value.all'Access;
    Hidden_Context : Views.Hidden := A.all'Access;
    Type_Prefix : Action := Action.all'Access;
begin
    null;
end CallbackDereferenceErrors;
