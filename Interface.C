#include <map>
#include <tuple>

#include "Interface.H"
#include "Utilities.H"
#include "faceTriangulation.H"
#include "cellSet.H"

#include <thread>
#include <chrono>


using namespace Foam;

preciceAdapter::Interface::Interface(
    precice::Participant& Precice,
    const fvMesh& Mesh,
    std::string MeshName,
    std::string LocationsType,
    std::vector<std::string> PatchNames,
    std::vector<std::string> CellSetNames,
    bool MeshConnectivity,
    bool RestartFromDeformed,
    const std::string& NamePointDisplacement,
    const std::string& NameCellDisplacement,
    std::string ConnectionName)
: mPrecice(Precice),
  mMeshName(MeshName),
  mPatchNames(PatchNames),
  mCellSetNames(CellSetNames),
  mMeshConnectivity(MeshConnectivity),
  mRestartFromDeformed(RestartFromDeformed),
  mConnectionName(ConnectionName)
{
    // mDim = mPrecice.getMeshDimensions(MeshName);
    mDim = 3;

    if (mDim == 2 && mMeshConnectivity == true)
    {
        DEBUG(adapterInfo("MeshConnectivity is currently only supported for 3D cases. \n"
                          "You might set up a 3D case and restrict the 3rd dimension by z-dead = true. \n"
                          "Have a look in the adapter documentation for detailed information.",
                          "warning"));
    }

    if (LocationsType == "FaceCenters" || LocationsType == "FaceCentres")
    {
        mLocationType = LocationType::FaceCenters;
    }
    else if (LocationsType == "FaceNodes")
    {
        mLocationType = LocationType::FaceNodes;
    }
    else if (LocationsType == "VolumeCenters" || LocationsType == "VolumeCentres")
    {
        mLocationType = LocationType::VolumeCenters;
    }
    else
    {
        adapterInfo("Interface points location type \""
                    "locations = "
                        + LocationsType + "\" is invalid.",
                    "error-deferred");
    }


    // For every patch that participates in the coupling
    for (uint j = 0; j < PatchNames.size(); j++)
    {
        // Get the patch_id
        int patch_id = Mesh.boundaryMesh().findPatchID(PatchNames.at(j));

        // Throw an error if the patch was not found
        if (patch_id == -1)
        {
            adapterInfo("Patch \""
                            + PatchNames.at(j) + "\" does not exist and therefore cannot be used as a coupling interface for Mesh \""
                            + MeshName + "\". Check the system/preciceDict.",
                        "error");
        }

        // Add the patch in the list
        mPatchIDs.push_back(patch_id);
    }

    // Configure the Mesh (set the data locations)
    ConfigureMesh(Mesh, NamePointDisplacement, NameCellDisplacement);
}

void preciceAdapter::Interface::ConfigureMesh(const fvMesh& Mesh, const std::string& NamePointDisplacement, const std::string& NameCellDisplacement)
{
    // The way we configure the Mesh differs between meshes based on face centers
    // and meshes based on face nodes.
    // TODO: Reduce code duplication. In the meantime, take care to update
    // all the branches.
    
    // Make CoSimIO::ModelPart and push in the array of model_part_interfaces
    mpModelPart = CoSimIO::make_unique<CoSimIO::ModelPart>(mMeshName);

    if (mLocationType == LocationType::FaceCenters)
    {
        // Count the data locations for all the patches
        for (uint j = 0; j < mPatchIDs.size(); j++)
        {
            mNumDataLocations +=
                Mesh.boundaryMesh()[mPatchIDs.at(j)].faceCentres().size();
        }
        DEBUG(adapterInfo("Number of face centres: " + std::to_string(mNumDataLocations)));

        // In case we want to perform the reset later on, look-up the corresponding data field name
        Foam::volVectorField const* cell_displacement = nullptr;
        if (Mesh.foundObject<volVectorField>(NameCellDisplacement))
            cell_displacement =
                &Mesh.lookupObject<volVectorField>(NameCellDisplacement);

        // Array of the Mesh vertices.
        // One Mesh is used for all the patches and each vertex has 3D coordinates.
        std::vector<double> vertices(mDim * mNumDataLocations);

        // Array of the indices of the Mesh vertices.
        // Each vertex has one index, but three coordinates.
        mVertexIDs.resize(mNumDataLocations);

        // Initialize the index of the vertices array
        int vertices_index = 0;

        // Get the locations of the Mesh vertices (here: face centers)
        // for all the patches
        int node_id = 1;
        for (uint j = 0; j < mPatchIDs.size(); j++)
        {
            // Get the face centers of the current patch
            vectorField FaceCenters =
                Mesh.boundaryMesh()[mPatchIDs.at(j)].faceCentres();

            // Move the interface according to the current values of the cell_displacement field,
            // to account for any displacements accumulated before restarting the simulation.
            // This is information that OpenFOAM reads from its result/restart files.
            // If the simulation is not restarted, the displacement should be zero and this line should have no effect.
            if (cell_displacement != nullptr && !mRestartFromDeformed)
                FaceCenters -= cell_displacement->boundaryField()[mPatchIDs.at(j)];

            // Assign the (x,y,z) locations to the vertices
            // id = 0
            for (int i = 0; i < FaceCenters.size(); i++){
                for (unsigned int d = 0; d < mDim; ++d)
                {
                    vertices[vertices_index++] = FaceCenters[i][d];
                }

                mVertexIDs[node_id - 1] = node_id;

                mpModelPart->CreateNewNode(
                    node_id,
                    FaceCenters[i][0],
                    FaceCenters[i][1],
                    FaceCenters[i][2]
                );

                node_id++;
            }
            

            // Check if we are in the right layer in case of preCICE dimension 2
            // If there is at least one node with a different z-coordinate, then the (2D) geometry is not on the xy-plane, as required.
            if (mDim == 2)
            {
                const pointField FaceNodes =
                    Mesh.boundaryMesh()[mPatchIDs.at(j)].localPoints();
                const auto FaceNodesSize = FaceNodes.size();
                //Allocate memory for z-coordinates
                std::array<double, 2> z_location({0, 0});
                constexpr unsigned int z_axis = 2;

                // Find out about the existing planes
                // Store z-coordinate of the first layer
                if (FaceNodesSize > 0)
                {
                    z_location[0] = FaceNodes[0][z_axis];
                }
                // Go through the remaining points until we find the second z-coordinate
                // and store it (there are only two allowed in case we are in the xy-layer)
                for (int i = 0; i < FaceNodesSize; i++)
                {
                    if (z_location[0] == FaceNodes[i][z_axis])
                    {
                        continue;
                    }
                    else
                    {
                        z_location[1] = FaceNodes[i][z_axis];
                        break;
                    }
                }

                // Check if the z-coordinates of all nodes match the z-coordinates we have collected above
                for (int i = 0; i < FaceNodesSize; i++)
                {
                    if (z_location[0] == FaceNodes[i][z_axis] || z_location[1] == FaceNodes[i][z_axis])
                    {
                        continue;
                    }
                    else
                    {
                        adapterInfo("It seems like you are using preCICE in 2D and your geometry is not located int the xy-plane. "
                                    "The OpenFOAM adapter implementation supports preCICE 2D cases only with the z-axis as out-of-plane direction."
                                    "Please rotate your geometry so that the geometry is located in the xy-plane."
                                    "If you are running a 2D axisymmetric case just ignore this.",
                                    "warning");
                    }
                }
            }
        }

        std::cout << "OPENFOAM: before Mesh export, sleeping..." << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(3));

        std::cout << "OPENFOAM: now exporting Mesh" << std::endl;


        // Pass the Mesh vertices information to preCICE
        //mPrecice.setMeshVertices(mMeshName, vertices, mVertexIDs);
        // For CoSimIO
        mInfo.Clear();
        mInfo.Set("identifier", mMeshName);
        mInfo.Set("connection_name", mConnectionName);
        auto export_info = CoSimIO::ExportMesh(mInfo, *mpModelPart);
        std::cout << "ExportMesh succesful!";
    }
    else if (mLocationType == LocationType::FaceNodes)
    {
        // Count the data locations for all the patches
        for (uint j = 0; j < mPatchIDs.size(); j++)
        {
            mNumDataLocations +=
                Mesh.boundaryMesh()[mPatchIDs.at(j)].localPoints().size();
        }
        DEBUG(adapterInfo("Number of face nodes: " + std::to_string(mNumDataLocations)));

        // In case we want to perform the reset later on, look-up the corresponding data field name
        Foam::pointVectorField const* pointDisplacement = nullptr;
        if (Mesh.foundObject<pointVectorField>(NamePointDisplacement))
            pointDisplacement =
                &Mesh.lookupObject<pointVectorField>(NamePointDisplacement);

        // Array of the Mesh vertices.
        // One Mesh is used for all the patches and each vertex has 3D coordinates.
        std::vector<double> vertices(mDim * mNumDataLocations);

        // Array of the indices of the Mesh vertices.
        // Each vertex has one index, but three coordinates.
        mVertexIDs.resize(mNumDataLocations);

        // Initialize the index of the vertices array
        int vertices_index = 0;

        // Map between OpenFOAM vertices and preCICE vertex IDs
        std::map<std::tuple<double, double, double>, int> verticesMap;

        // Get the locations of the Mesh vertices (here: face nodes)
        // for all the patches
        int node_id = 1;
        for (uint j = 0; j < mPatchIDs.size(); j++)
        {
            // Get the face nodes of the current patch
            // TODO: Check if this is correct.
            // TODO: Check if this behaves correctly in parallel.
            // TODO: Check if this behaves correctly with multiple, connected patches.
            // TODO: Maybe this should be a pointVectorField?
            pointField FaceNodes =
                Mesh.boundaryMesh()[mPatchIDs.at(j)].localPoints();

            // Similar to the cell displacement above:
            // Move the interface according to the current values of the cell_displacement field,
            // to account for any displacements accumulated before restarting the simulation.
            // This is information that OpenFOAM reads from its result/restart files.
            // If the simulation is not restarted, the displacement should be zero and this line should have no effect.
            if (pointDisplacement != nullptr && !mRestartFromDeformed)
            {
                const vectorField& resetField = refCast<const vectorField>(
                    pointDisplacement->boundaryField()[mPatchIDs.at(j)]);
                FaceNodes -= resetField;
            }

            // Assign the (x,y,z) locations to the vertices
            // TODO: Ensure consistent order when writing/reading
            for (int i = 0; i < FaceNodes.size(); i++)
            {
                for (unsigned int d = 0; d < mDim; ++d)
                {
                    vertices[vertices_index++] = FaceNodes[i][d];
                }

                mVertexIDs[node_id - 1] = node_id;
                
                // Pass the Mesh vertices informtion to CoSimIO
                mpModelPart->CreateNewNode( node_id, FaceNodes[i][0], FaceNodes[i][1], FaceNodes[i][2]);

                node_id++;
            }
        }

        // For CoSimIO
        mInfo.Clear();
        mInfo.Set("identifier", mMeshName);
        mInfo.Set("connection_name", mConnectionName);
        auto export_info = CoSimIO::ExportMesh(mInfo, *mpModelPart);
        std::cout << "ExportMesh succesful!";
        //exit(0);
        //debugInfo( "Finished Exporting interface Mesh " +  mMeshName + " to Kratos as a ModelPart "  , debugLevel);

        // Pass the Mesh vertices information to preCICE
        //mPrecice.setMeshVertices(mMeshName, vertices, mVertexIDs);

        if (mMeshConnectivity)
        {
            for (std::size_t i = 0; i < mVertexIDs.size(); ++i)
            {
                verticesMap.emplace(std::make_tuple(vertices[3 * i], vertices[3 * i + 1], vertices[3 * i + 2]), mVertexIDs[i]);
            }

            for (uint j = 0; j < mPatchIDs.size(); j++)
            {
                // Define triangles
                // This is done in the following way:
                // We get a list of faces, which belong to this patch, and triangulate each face
                // using the faceTriangulation object.
                // Afterwards, we store the coordinates of the triangulated faces in order to use
                // the preCICE function "getMeshVertexIDsFromPositions". This function returns
                // for each point the respective preCICE related ID.
                // These IDs are consequently used for the preCICE function "setMeshTriangleWithEdges",
                // which defines edges and triangles on the interface. This connectivity information
                // allows preCICE to provide a nearest-projection mapping.
                // Since data is now related to nodes, volume fields (e.g. heat flux) needs to be
                // interpolated in the data classes (e.g. CHT)

                // Define constants
                const int triaPerQuad = 2;
                const int nodesPerTria = 3;

                // Get the list of faces and coordinates at the interface patch
                const List<face> faceField = Mesh.boundaryMesh()[mPatchIDs.at(j)].localFaces();
                Field<point> pointCoords = Mesh.boundaryMesh()[mPatchIDs.at(j)].localPoints();

                // Subtract the displacement part in case we have deformation
                if (pointDisplacement != nullptr && !mRestartFromDeformed)
                {
                    const vectorField& resetField = refCast<const vectorField>(
                        pointDisplacement->boundaryField()[mPatchIDs.at(j)]);
                    pointCoords -= resetField;
                }

                //Array to store the IDs we get from preCICE
                std::vector<int> triVertIDs;
                triVertIDs.reserve(faceField.size() * triaPerQuad * nodesPerTria);

                // Triangulate all faces and collect set of nodes that form triangles,
                // which are used to set Mesh triangles in preCICE.
                forAll(faceField, facei)
                {
                    const face& faceQuad = faceField[facei];

                    // Triangulate the face
                    faceTriangulation faceTri(pointCoords, faceQuad, false);

                    // Iterate over all triangles generated out of each (quad) face
                    for (uint triIndex = 0; triIndex < triaPerQuad; triIndex++)
                    {
                        // Get the vertex that corresponds to the x,y,z coordinates of each node of a triangle
                        for (uint nodeIndex = 0; nodeIndex < nodesPerTria; nodeIndex++)
                        {
                            triVertIDs.push_back(verticesMap.at(std::make_tuple(pointCoords[faceTri[triIndex][nodeIndex]][0], pointCoords[faceTri[triIndex][nodeIndex]][1], pointCoords[faceTri[triIndex][nodeIndex]][2])));
                        }
                    }
                }

                DEBUG(adapterInfo("Number of triangles: " + std::to_string(faceField.size() * triaPerQuad)));

                //Set Triangles
                //mPrecice.setMeshTriangles(mMeshName, triVertIDs);
            }
        }
    }
    else if (mLocationType == LocationType::VolumeCenters)
    {
        // The volume coupling implementation considers the Mesh points in the volume and
        // on the boundary patches in order to take the boundary conditions into account

        // Get the cell labels of the overlapping region
        std::vector<labelList> overlapCells;

        if (!mCellSetNames.empty())
        {
            // For every cellSet that participates in the coupling
            for (uint j = 0; j < mCellSetNames.size(); j++)
            {
                // Create a cell set
                cellSet overlapRegion(Mesh, mCellSetNames[j]);

                // Add the cells IDs to the vector and count how many overlap cells the interface has
                overlapCells.push_back(overlapRegion.toc());
                mNumDataLocations += overlapCells[j].size();
            }
        }
        else
        {
            mNumDataLocations = Mesh.C().size();
        }

        // Count the data locations for all the patches
        // and add those to the previously determined number of Mesh points in the volume
        for (uint j = 0; j < mPatchIDs.size(); j++)
        {
            mNumDataLocations +=
                Mesh.boundaryMesh()[mPatchIDs.at(j)].faceCentres().size();
        }
        DEBUG(adapterInfo("Number of coupling volumes: " + std::to_string(mNumDataLocations)));

        // Array of the Mesh vertices.
        // One Mesh is used for all the patches and each vertex has 3D coordinates.
        std::vector<double> vertices(mDim * mNumDataLocations);

        // Array of the indices of the Mesh vertices.
        // Each vertex has one index, but three coordinates.
        mVertexIDs.resize(mNumDataLocations);

        // Initialize the index of the vertices array
        int node_id = 1;
        if (!mCellSetNames.empty())
        {
            // for all the overlapping cells (cellSets)
            for (uint j = 0; j < mCellSetNames.size(); j++)
            {
                // Get the cell centres of the current cellSet.
                const labelList& cells = overlapCells.at(j);

                // Get the coordinates of the cells of the current cellSet.
                for (int i = 0; i < cells.size(); i++)
                {
                    // vertices[vertices_index++] = Mesh.C().internalField()[cells[i]].x();
                    // vertices[vertices_index++] = Mesh.C().internalField()[cells[i]].y();
                    // if (mDim == 3)
                    // {
                    //     vertices[vertices_index++] = Mesh.C().internalField()[cells[i]].z();
                    // }
                    mpModelPart->CreateNewNode(node_id, Mesh.C().internalField()[cells[i]].x(), Mesh.C().internalField()[cells[i]].y(), Mesh.C().internalField()[cells[i]].z());    
                    mVertexIDs[node_id - 1] = node_id;
                    node_id++;
                }
            }
        }
        else
        {
            const vectorField& CellCenters = Mesh.C();

            for (int i = 0; i < CellCenters.size(); i++)
            {
                // vertices[vertices_index++] = CellCenters[i].x();
                // vertices[vertices_index++] = CellCenters[i].y();
                // if (mDim == 3)
                // {
                //     vertices[vertices_index++] = CellCenters[i].z();
                // }
                mpModelPart->CreateNewNode(node_id, CellCenters[i].x(), CellCenters[i].y(), CellCenters[i].z());    
                mVertexIDs[node_id - 1] = node_id;
                node_id++;
            }
            
        }

        // Get the locations of the Mesh vertices (here: face centers)
        // for all the patches
        for (uint j = 0; j < mPatchIDs.size(); j++)
        {
            // Get the face centers of the current patch
            const vectorField FaceCenters =
                Mesh.boundaryMesh()[mPatchIDs.at(j)].faceCentres();

            // Assign the (x,y,z) locations to the vertices
            for (int i = 0; i < FaceCenters.size(); i++)
            {
                // vertices[vertices_index++] = FaceCenters[i].x();
                // vertices[vertices_index++] = FaceCenters[i].y();
                // if (mDim == 3)
                // {
                //     vertices[vertices_index++] = FaceCenters[i].z();
                // }
                mpModelPart->CreateNewNode(node_id, FaceCenters[i][0], FaceCenters[i][1], FaceCenters[i][2]);    
                mVertexIDs[node_id - 1] = node_id;
                node_id++;
            }
            
        }

        // Pass the Mesh vertices information to preCICE
        //recice_.setMeshVertices(mMeshName, vertices, mVertexIDs);
        // For CoSimIO
        mInfo.Clear();
        mInfo.Set("identifier", mMeshName);
        mInfo.Set("connection_name", mConnectionName);
        auto export_info = CoSimIO::ExportMesh(mInfo, *mpModelPart);
        std::cout << "ExportMesh succesful!";
    }
}


void preciceAdapter::Interface::AddCouplingDataWriter(
    const FieldConfig& fieldConfig,
    CouplingDataUser* couplingDataWriter)
{
    // Set the data name (from preCICE)
    couplingDataWriter->SetDataName(fieldConfig.name);

    // Set the flip normal option
    couplingDataWriter->SetFlipNormal(fieldConfig.flip_normal);

    // Set the patchIDs of the patches that form the interface
    couplingDataWriter->SetPatchIDs(mPatchIDs);

    // Set the names of the cell sets to be coupled (for volume coupling)
    couplingDataWriter->SetCellSetNames(mCellSetNames);

    // Set the location type in the CouplingDataUser class
    couplingDataWriter->SetLocationsType(mLocationType);

    // Set the location type in the CouplingDataUser class
    couplingDataWriter->CheckDataLocation(mMeshConnectivity);

    // Initilaize class specific data
    couplingDataWriter->Initialize();

    // Add the CouplingDataUser to the list of writers
    mCouplingDataWriters.push_back(couplingDataWriter);
}


void preciceAdapter::Interface::AddCouplingDataReader(
    const FieldConfig& fieldConfig,
    preciceAdapter::CouplingDataUser* couplingDataReader)
{
    // Set the patchIDs of the patches that form the interface
    couplingDataReader->SetDataName(fieldConfig.name);

    // Set the flip normal option
    couplingDataReader->SetFlipNormal(fieldConfig.flip_normal);

    // Add the CouplingDataUser to the list of readers
    couplingDataReader->SetPatchIDs(mPatchIDs);

    // Set the location type in the CouplingDataUser class
    couplingDataReader->SetLocationsType(mLocationType);

    // Set the names of the cell sets to be coupled (for volume coupling)
    couplingDataReader->SetCellSetNames(mCellSetNames);

    // Check, if the current location type is supported by the data type
    couplingDataReader->CheckDataLocation(mMeshConnectivity);

    // Initilaize class specific data
    couplingDataReader->Initialize();

    // Add the CouplingDataUser to the list of readers
    mCouplingDataReaders.push_back(couplingDataReader);
}

void preciceAdapter::Interface::CreateBuffer()
{
    // Will the interface buffer need to store 3D vector data?
    bool needsVectorData = false;
    int dataBufferSize = 0;

    // Check all the coupling data readers
    for (uint i = 0; i < mCouplingDataReaders.size(); i++)
    {
        if (mCouplingDataReaders.at(i)->HasVectorData())
        {
            needsVectorData = true;
        }
    }

    // Check all the coupling data writers
    for (uint i = 0; i < mCouplingDataWriters.size(); i++)
    {
        if (mCouplingDataWriters.at(i)->HasVectorData())
        {
            needsVectorData = true;
        }
    }

    // Set the appropriate buffer size
    if (needsVectorData)
    {
        dataBufferSize = mDim * mNumDataLocations;
    }
    else
    {
        dataBufferSize = mNumDataLocations;
    }

    // Create the data buffer
    // An interface has only one data buffer, which is shared between several
    // CouplingDataUsers.
    mDataBuffer.resize(dataBufferSize);
}

void preciceAdapter::Interface::ReadCouplingData(double relativeReadTime)
{
    (void)relativeReadTime;

    // Make every coupling data reader Read
    for (uint i = 0; i < mCouplingDataReaders.size(); i++)
    {
        // Pointer to the current reader
        preciceAdapter::CouplingDataUser*
            couplingDataReader = mCouplingDataReaders.at(i);

        // Determine expected size
        std::size_t nReadData =
            mVertexIDs.size()
            * (couplingDataReader->HasVectorData() ? mDim : 1);

        // Prepare CoSimIO import
        CoSimIO::Info import_info;
        import_info.Set("connection_name", mConnectionName);
        import_info.Set("identifier", couplingDataReader->DataName());

        // Receive data
        std::vector<double> received_data(nReadData);

        import_info = CoSimIO::ImportData(
            import_info,
            received_data);

        // Optional sanity check
        if (received_data.size() != nReadData)
        {
            std::cerr << "ERROR: expected "
                      << nReadData
                      << " values but received "
                      << received_data.size()
                      << std::endl;
        }

        // Copy into adapter buffer
        std::copy(
            received_data.begin(),
            received_data.end(),
            mDataBuffer.begin());

        // Apply flip normal if required
        couplingDataReader->ApplyFlipNormal(
            mDataBuffer.data(),
            received_data.size());

        // Read the received data from the buffer
        couplingDataReader->Read(
            mDataBuffer.data(),
            mDim);
    }
}

void preciceAdapter::Interface::WriteCouplingData()
{
    std::cout << "INSIDE WriteCouplingData()" << std::endl;

    std::cout << "Number of writers = "
              << mCouplingDataWriters.size()
              << std::endl;
              
    // Make every coupling data writer Write
    for (uint i = 0; i < mCouplingDataWriters.size(); i++)
    {
        // Pointer to the current reader
        preciceAdapter::CouplingDataUser*
            couplingDataWriter = mCouplingDataWriters.at(i);

        // Write the data into the adapter's buffer
        auto nWrittenData = couplingDataWriter->Write(mDataBuffer.data(), mMeshConnectivity, mDim);

        // Apply flip normal if required
        couplingDataWriter->ApplyFlipNormal(
            mDataBuffer.data(),
            nWrittenData);

        // Convert span/buffer into std::vector<double> for CoSimIO
        std::vector<double> data_to_send(
            mDataBuffer.begin(),
            mDataBuffer.begin() + nWrittenData
        );
        
        std::cout << data_to_send.size() << std::endl;
        for (auto force : data_to_send)
        {
            std::cout << force << std::endl;
        }

        CoSimIO::Info export_info;
        export_info.Set("connection_name", mConnectionName);
        export_info.Set("identifier", couplingDataWriter->DataName());

        export_info = CoSimIO::ExportData(
            export_info,
            data_to_send
        );
    }
}

preciceAdapter::Interface::~Interface()
{
    // Delete all the coupling data readers
    for (uint i = 0; i < mCouplingDataReaders.size(); i++)
    {
        delete mCouplingDataReaders.at(i);
    }
    mCouplingDataReaders.clear();

    // Delete all the coupling data writers
    for (uint i = 0; i < mCouplingDataWriters.size(); i++)
    {
        delete mCouplingDataWriters.at(i);
    }
    mCouplingDataWriters.clear();
}
