extends Node

const Inner := preload("res://tests/statics.gd")
var a: Array[Node]             # 原生类型
var b: Array[String]           # 内置类型
var c: Array[Inner]            # 脚本类型  <- 这里
var d := preload("res://tests/statics.gd")   # 对象常量  <- 和这里
