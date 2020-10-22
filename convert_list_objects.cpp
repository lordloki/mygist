void KX_Scene::convert_blender_objects_list_synchronous(Object *objectslist)
{
  KX_KetsjiEngine *engine = KX_GetActiveEngine();
  e_PhysicsEngine physics_engine = UseBullet;
  RAS_Rasterizer *rasty = engine->GetRasterizer();
  RAS_ICanvas *canvas = engine->GetCanvas();
  bContext *C = engine->GetContext();
  Depsgraph *depsgraph = CTX_data_expect_evaluated_depsgraph(C);
  Main *bmain = CTX_data_main(C);

  LISTBASE_FOREACH (Object *, obj, &objectslist) {
    BL_ConvertBlenderObjects(bmain,
                             depsgraph,
                             this,
                             engine,
                             physics_engine,
                             rasty,
                             canvas,
                             m_sceneConverter,
                             obj,
                             false,
                             false);
  }
  LISTBASE_FOREACH_END;
}

// Task data for convertBlenderCollection in a different thread.
struct ConvertBlenderObjectsListTaskData {
  Object *objectslist;
  KX_KetsjiEngine *engine;
  e_PhysicsEngine physics_engine;
  KX_Scene *scene;
  BL_BlenderSceneConverter *converter;
};

void convert_blender_objects_list_thread_func(TaskPool *__restrict UNUSED(pool),
                                            void *taskdata,
                                            int UNUSED(threadid))
{
  ConvertBlenderObjectsListTaskData *task = static_cast<ConvertBlenderObjectsListTaskData *>(taskdata);

  RAS_Rasterizer *rasty = task->engine->GetRasterizer();
  RAS_ICanvas *canvas = task->engine->GetCanvas();
  bContext *C = task->engine->GetContext();
  Depsgraph *depsgraph = CTX_data_expect_evaluated_depsgraph(C);
  Main *bmain = CTX_data_main(C);

  LISTBASE_FOREACH (Object *, obj, &task->objectslist) {
    BL_ConvertBlenderObjects(bmain,
                             depsgraph,
                             task->scene,
                             task->engine,
                             task->physics_engine,
                             rasty,
                             canvas,
                             task->converter,
                             obj,
                             false,
                             false);
  }
  LISTBASE_FOREACH_END;
}

void KX_Scene::ConvertBlenderObjectsList(Object *objectslist, bool asynchronous)
{
  if (asynchronous) {
    TaskPool *taskpool = BLI_task_pool_create(nullptr, TASK_PRIORITY_LOW);

    /* Convert the Blender collection in a different thread, so that the
     * game engine can keep running at full speed. */
    ConvertBlenderObjectsListTaskData *task = (ConvertBlenderObjectsListTaskData *)MEM_mallocN(sizeof(ConvertBlenderObjectsListTaskData),
                                                                                             "convertblenderobjectslist-data");

    task->engine = KX_GetActiveEngine();
    task->physics_engine = UseBullet;
    task->objectslist = objectslist;
    task->scene = this;
    task->converter = m_sceneConverter;

    BLI_task_pool_push(taskpool,
                       (TaskRunFunction)convert_blender_objects_list_thread_func,
                       task,
                       true,  // free task data
                       NULL);
    BLI_task_pool_work_and_wait(taskpool);
    BLI_task_pool_free(taskpool);
    taskpool = nullptr;
  }
  else {
    convert_blender_objects_list_synchronous(objectslist);
  }
}


KX_PYMETHODDEF_DOC(KX_Scene,
                   convertBlenderObjectsList,
                   "convertBlenderObjectsList()\n"
                   "\n")
{
  std:vector<Object> objectlist;
  PyObject *list;
  int asynchronous = 0;

  if (!PyArg_ParseTuple(args, "O!i:", &PyList_Type, &list, &asynchronous)) {
    std::cout << "Expected a bpy.types.Object list." << std::endl;
    return nullptr;
  }
  
  std:vector<Object> objectslist;
  
  for (Py_ssize_t i = 0; i < list_size; i++) {
    PyObject* bl_object = PyList_GetItem(list, i);

    ID *id;
    if (!pyrna_id_FromPyObject(bl_object, &id)) {
      std::cout << "Failed to convert object." << std::endl;
      return nullptr;
    }

    objectslist.push_back((Object *)id);
  }

  ConvertBlenderObjectsList(&objectlist.front(), asynchronous);
  Py_RETURN_NONE;
}
