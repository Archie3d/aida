procedure GenericInterfaceErrors is
    type I is interface;
    type J is interface;
    type Both is interface and J and I;
    type L is limited interface;
    type S is synchronized interface;
    type T is task interface;
    type P is protected interface;
    type Item is new I with null record;
    generic
        type F is interface;
    package Ordinary is
    end Ordinary;
    generic
        type F is limited interface;
    package Limited_Formal is
    end Limited_Formal;
    generic
        type F is synchronized interface;
    package Synchronized_Formal is
    end Synchronized_Formal;
    generic
        type F is task interface;
    package Task_Formal is
    end Task_Formal;
    generic
        type F is protected interface;
    package Protected_Formal is
    end Protected_Formal;
    generic
        type F is interface and I and J;
    package Ancestors is
    end Ancestors;
    package Wrong_Concrete is new Ordinary (Item);
    package Wrong_Limited is new Ordinary (L);
    package Wrong_Ordinary is new Limited_Formal (I);
    package Wrong_Synchronized is new Limited_Formal (S);
    package Wrong_Task is new Synchronized_Formal (T);
    package Wrong_Protected is new Task_Formal (P);
    package Wrong_Sync_Task is new Task_Formal (S);
    package Wrong_Protected_Sync is new Protected_Formal (S);
    package Wrong_Classwide is new Ordinary (I'Class);
    package Missing_Ancestor is new Ancestors (I);
    package Good_Ancestors is new Ancestors (Both);
    package Good_Ordinary is new Ordinary (Both);
    package Good_Limited is new Limited_Formal (L);
    package Good_Sync is new Synchronized_Formal (S);
    package Good_Task is new Task_Formal (T);
    package Good_Protected is new Protected_Formal (P);
    generic
        type F is limited interface;
        type D is new F with private;
    package Nonlimited_Derived is
    end Nonlimited_Derived;
    type Limited_Item is limited new L with null record;
    package Wrong_Derived_Limited is new Nonlimited_Derived (L, Limited_Item);
    package Model is
        type Contract is interface;
        procedure Ping (X : Contract) is null;
        type Changed is interface and Contract;
        overriding procedure Ping (X : Changed) is abstract;
    end Model;
    generic
        type F is interface and Model.Contract;
    package Concrete_Contract is
    end Concrete_Contract;
    package Bad_Concrete_Operation is new Concrete_Contract (Model.Changed);
    package Good_Concrete_Operation is new Concrete_Contract (Model.Contract);
    generic
        type F is interface;
    package Invalid is
        Object : F;
        type Array_Type is array (1 .. 2) of F;
        type Record_Type is record
            Component : F;
        end record;
        function Make return F;
    end Invalid;
    generic
        type F is interface;
        Object : F;
    package Invalid_Object is
    end Invalid_Object;
begin
    null;
end GenericInterfaceErrors;
