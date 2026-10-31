with Ada.Finalization;
procedure ProtectedLegalityErrors is
    protected type Counter is
        procedure Add (amount : Integer);
    private
        value : Integer := 0;
    end Counter;
    protected body Counter is
        procedure Add (differentName : Integer) is
        begin
            null;
        end Add;
    end Counter;
    protected body Counter is
    end Counter;
    subtype AliasCounter is Counter;
    protected body AliasCounter is
    end AliasCounter;
    object : Counter;
    protected body object is
    end object;
    protected body Orphan is
    end Orphan;
    protected type BadPrivate is
    private
        other : Counter;
    end BadPrivate;
    type TaggedHolder is new Ada.Finalization.Limited_Controlled with record
        item : Counter;
    end record;
    package BadCompletion is
        type Hidden is private;
    private
        protected type Hidden is
        end Hidden;
    end BadCompletion;
    package HiddenModel is
        type Hidden is limited private;
    private
        protected type Hidden is
            function Get return Integer;
        end Hidden;
    end HiddenModel;
    package body HiddenModel is
        protected body Hidden is
            function Get return Integer is
            begin
                return 0;
            end Get;
        end Hidden;
    end HiddenModel;
    hiddenObject : HiddenModel.Hidden;
    hiddenValue : Integer := hiddenObject.Get;
begin
    null;
end ProtectedLegalityErrors;
